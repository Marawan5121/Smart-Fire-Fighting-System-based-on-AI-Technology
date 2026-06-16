#ifndef FILTERS_H
#define FILTERS_H

/**
 * @brief Exponential Moving Average (EMA) Filter class.
 * Provides real-time noise filtering for analog signals with O(1) memory overhead.
 */
class EmaFilter {
private:
    float _alpha;       // Smoothing factor (0.0 to 1.0)
    float _filtered;    // Current filtered state
    bool _isInitialized; // Flag to check if first reading has occurred

public:
    /**
     * @brief Construct a new Ema Filter object
     * @param alpha Smoothing factor (lower = smoother, higher = faster response)
     */
    EmaFilter(float alpha) : _alpha(alpha), _filtered(0.0f), _isInitialized(false) {}

    /**
     * @brief Inputs a raw reading and outputs the new filtered value
     * @param rawValue The raw sensor reading (e.g. from analogRead)
     * @return float The smoothed value
     */
    float filter(float rawValue) {
        if (!_isInitialized) {
            _filtered = rawValue; // Initialize filter with the very first reading
            _isInitialized = true;
        } else {
            // Formula: Y_t = α * X_t + (1 - α) * Y_{t-1}
            _filtered = (_alpha * rawValue) + ((1.0f - _alpha) * _filtered);
        }
        return _filtered;
    }

    /**
     * @brief Get the current filtered value without updating
     * @return float Current value
     */
    float getValue() const {
        return _filtered;
    }

    /**
     * @brief Resets the filter initialization state
     */
    void reset() {
        _isInitialized = false;
        _filtered = 0.0f;
    }

    /**
     * @brief Update the alpha coefficient dynamically
     * @param newAlpha New smoothing factor
     */
    void setAlpha(float newAlpha) {
        if (newAlpha >= 0.0f && newAlpha <= 1.0f) {
            _alpha = newAlpha;
        }
    }
};

#endif // FILTERS_H
