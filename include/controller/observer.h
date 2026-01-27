
enum class ModelEvent {
    TURN_CHANGE,
};

/**
 * Interface for observers
 */
class ModelObserver {
public:
    virtual ~ModelObserver() = default;

    /**
     * Notify the observer that the model has changed
     */
    virtual void onModelChanged(ModelEvent event) = 0;
};