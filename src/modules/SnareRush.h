#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <array>

/**
 * Snare Rush — M4
 *
 * Synthesized: bandpassed noise (1.5–4 kHz) + sine body at ~180 Hz
 * AD envelope per hit: ~1ms attack, 60–120ms decay.
 * Tempo-synced trigger: subdivision ladder [8, 16, 32, 64].
 * Activation controls:
 *   - amplitude (via smoothstep)
 *   - subdivision density (higher activation → denser)
 * Outputs to a separate stereo GENERATOR buffer added in processBlock.
 */
class SnareRush
{
public:
    SnareRush() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        // Bandpass for noise (center ~ 2.5kHz, wide-ish)
        noiseBP.prepare(spec);
        noiseBP.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        noiseBP.setCutoffFrequency(2500.0f);
        noiseBP.setResonance(1.2f);

        reset();
    }

    void reset()
    {
        noiseEnv   = 0.0f;
        bodyEnv    = 0.0f;
        bodyPhase  = 0.0f;
        sampleCounter = 0;
        intervalSamples = 0;
        attackSamples = 0;
        attackCounter = 0;
        inAttack = false;
    }

    /**
     * Set BPM for subdivision calculation.
     */
    void setBpm(double bpm)
    {
        currentBpm = bpm;
    }

    /**
     * Set subdivision (e.g. 8, 16, 32, 64).
     */
    void setSubdivision(int div)
    {
        currentDiv = juce::jlimit(1, 256, div);
    }

    /** Returns the currently active subdivision integer (e.g. 8, 16, 32, 64). */
    int getCurrentDiv() const { return currentDiv; }

    /**
     * Set tempo and activation level.
     * bpm: current host BPM (or 120 fallback)
     * activation: 0..1 from smoothstep macro
     * maxDiv: preset max subdivision (16, 32, 64, etc.)
     */
    void setParams(double bpm, float activation, int maxDiv)
    {
        currentActivation = activation;
        currentBpm = bpm;

        if (activation <= 0.001f)
        {
            intervalSamples = 0;
            return;
        }

        // Pick subdivision based on activation: 8→16→32→maxDiv
        // Ladder: 8 at low, maxDiv at high. Subdivide by factor of 2.
        // We use 3 divisions: 8, 16, and maxDiv (clamped to [16,64])
        static const int divLadder[4] = { 8, 16, 32, 64 };

        // Find the highest index where divLadder[i] <= maxDiv
        int maxIdx = 0;
        for (int i = 0; i < 4; ++i)
            if (divLadder[i] <= maxDiv) maxIdx = i;

        // Map activation 0..1 to index 0..maxIdx
        int divIdx = juce::roundToInt(activation * static_cast<float>(maxIdx));
        divIdx = juce::jlimit(0, maxIdx, divIdx);
        int subdivision = divLadder[divIdx];
        currentDiv = subdivision;

        // Interval in samples = (60 / bpm) * (4 / subdivision) * sampleRate
        double beatSec = 60.0 / bpm;
        double noteSec = beatSec * 4.0 / static_cast<double>(subdivision);
        intervalSamples = static_cast<int>(noteSec * sampleRate);
        if (intervalSamples < 64) intervalSamples = 64;

        // Attack = 1ms
        attackSamples = static_cast<int>(0.001 * sampleRate);
        if (attackSamples < 1) attackSamples = 1;

        // Decay coefficient for 60–80ms (mix based on activation)
        double decayMs = 120.0 - activation * 60.0; // 120ms at low, 60ms at high
        noiseDecay = static_cast<float>(std::pow(0.001, 1.0 / (decayMs * 0.001 * sampleRate)));
        bodyDecay  = static_cast<float>(std::pow(0.001, 1.0 / (80.0 * 0.001 * sampleRate)));
    }

    /**
     * Fill outputBuffer (add into it — does NOT clear it first).
     * Caller is responsible for clearing before calling generators.
     * Legacy overload using internal sample counter.
     */
    void process(juce::AudioBuffer<float>& outputBuffer, int numSamples)
    {
        if (currentActivation <= 0.001f || intervalSamples <= 0)
            return;

        const int numChannels = outputBuffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            // Trigger on interval
            if (sampleCounter >= intervalSamples)
            {
                sampleCounter = 0;
                triggerSnare();
            }
            ++sampleCounter;

            // Generate sample
            float snare = generateSample();
            float scaled = snare * currentActivation;

            for (int ch = 0; ch < numChannels; ++ch)
                outputBuffer.addSample(ch, i, scaled);
        }
    }

    /**
     * Transport-locked process overload using host PPQ position.
     * Triggers on exact beat subdivision boundaries, eliminating drift.
     */
    void process(juce::AudioBuffer<float>& outputBuffer, float activation, float depth,
                 double sr, double bpm, double ppqPositionAtBlockStart, int samplesInBlock)
    {
        if (activation <= 0.001f || currentDiv <= 0)
            return;

        // Update decay params if activation changed
        if (activation != currentActivation)
        {
            currentActivation = activation;
            double decayMs = 120.0 - activation * 60.0;
            noiseDecay = static_cast<float>(std::pow(0.001, 1.0 / (decayMs * 0.001 * sr)));
            bodyDecay  = static_cast<float>(std::pow(0.001, 1.0 / (80.0 * 0.001 * sr)));
            attackSamples = static_cast<int>(0.001 * sr);
            if (attackSamples < 1) attackSamples = 1;
        }

        // subdivision in quarter notes: e.g. 1/16 = 0.25 QN
        double subdivQN = 4.0 / static_cast<double>(currentDiv);
        const int numChannels = outputBuffer.getNumChannels();

        for (int i = 0; i < samplesInBlock; ++i)
        {
            // ppq position of this sample
            double ppq = ppqPositionAtBlockStart + (double)i * bpm / (sr * 60.0);

            // check if this sample crosses a subdivision boundary
            bool crossesBoundary = false;
            if (i == 0)
            {
                // For first sample, check against stored previous ppq
                crossesBoundary = (int)(ppq / subdivQN) > (int)(lastPpq / subdivQN);
            }
            else
            {
                double prevPpq = ppqPositionAtBlockStart + (double)(i - 1) * bpm / (sr * 60.0);
                crossesBoundary = (int)(ppq / subdivQN) > (int)(prevPpq / subdivQN);
            }

            if (crossesBoundary)
                triggerSnare();

            float snare = generateSample();
            float scaled = snare * activation * depth;

            for (int ch = 0; ch < numChannels; ++ch)
                outputBuffer.addSample(ch, i, scaled);
        }

        // Store last ppq for next block boundary check
        lastPpq = ppqPositionAtBlockStart + (double)(samplesInBlock - 1) * bpm / (sr * 60.0);
    }

private:
    void triggerSnare()
    {
        inAttack     = true;
        attackCounter = 0;
        noiseEnv     = 0.0f; // will ramp up in attack
        bodyEnv      = 0.0f;
        bodyPhase    = 0.0f;
    }

    float generateSample()
    {
        // Attack ramp
        float envGain = 1.0f;
        if (inAttack)
        {
            envGain = static_cast<float>(attackCounter + 1) / static_cast<float>(attackSamples);
            ++attackCounter;
            if (attackCounter >= attackSamples)
            {
                inAttack  = false;
                noiseEnv  = 1.0f;
                bodyEnv   = 1.0f;
            }
            else
            {
                noiseEnv = envGain;
                bodyEnv  = envGain;
            }
        }

        // Noise component (will be bandpassed after accumulation — applied per block)
        float noise = (random.nextFloat() * 2.0f - 1.0f) * noiseEnv;

        // Sine body ~180 Hz
        float body = std::sin(bodyPhase) * bodyEnv;
        bodyPhase += juce::MathConstants<float>::twoPi * 180.0f / static_cast<float>(sampleRate);
        if (bodyPhase >= juce::MathConstants<float>::twoPi)
            bodyPhase -= juce::MathConstants<float>::twoPi;

        // Decay
        if (!inAttack)
        {
            noiseEnv *= noiseDecay;
            bodyEnv  *= bodyDecay;
        }

        return noise * 0.7f + body * 0.3f;
    }

    juce::dsp::StateVariableTPTFilter<float> noiseBP;
    juce::Random random;

    double sampleRate        = 44100.0;
    float  currentActivation = 0.0f;
    double currentBpm        = 120.0;
    int    currentDiv        = 16;
    double lastPpq           = 0.0;

    float  noiseEnv       = 0.0f;
    float  bodyEnv        = 0.0f;
    float  bodyPhase      = 0.0f;
    float  noiseDecay     = 0.999f;
    float  bodyDecay      = 0.999f;

    int    sampleCounter    = 0;
    int    intervalSamples  = 0;
    int    attackSamples    = 44;
    int    attackCounter    = 0;
    bool   inAttack         = false;
};
