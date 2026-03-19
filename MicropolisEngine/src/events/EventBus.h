/**
 * @file EventBus.h
 * @brief Modern std::function-based event bus for decoupled subsystem communication.
 *
 * MODERNIZATION (Phase 3):
 * This event bus replaces direct method calls between subsystems with a
 * publish/subscribe pattern, enabling:
 * - Loose coupling between subsystems
 * - Multiple listeners per event type
 * - Easy testing with mock subscribers
 * - Runtime event subscription/unsubscription
 */

#ifndef MICROPOLIS_EVENT_BUS_H
#define MICROPOLIS_EVENT_BUS_H

#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <algorithm>

namespace MicropolisEngine {

/**
 * @brief Type-safe event bus using std::function for callbacks.
 *
 * Usage:
 * @code
 *   EventBus bus;
 *
 *   // Subscribe to events
 *   auto id = bus.subscribe<PowerScanComplete>([](const PowerScanComplete& e) {
 *       std::cout << "Power scan complete: " << e.poweredTiles << " tiles powered\n";
 *   });
 *
 *   // Publish events
 *   bus.publish(PowerScanComplete{1000, 2000});
 *
 *   // Unsubscribe when done
 *   bus.unsubscribe<PowerScanComplete>(id);
 * @endcode
 */
class EventBus {
public:
    using HandlerId = std::size_t;

    EventBus() : nextHandlerId_(1) {}

    /**
     * @brief Subscribe to events of a specific type.
     * @tparam EventType The event struct type to subscribe to.
     * @param handler Function to call when event is published.
     * @return Handler ID for later unsubscription.
     */
    template<typename EventType>
    HandlerId subscribe(std::function<void(const EventType&)> handler) {
        auto typeIndex = std::type_index(typeid(EventType));
        HandlerId id = nextHandlerId_++;

        // Wrap the typed handler in a type-erased wrapper
        auto wrapper = std::make_shared<TypedHandler<EventType>>(std::move(handler));
        handlers_[typeIndex].emplace_back(id, wrapper);

        return id;
    }

    /**
     * @brief Unsubscribe a handler by its ID.
     * @tparam EventType The event type the handler was subscribed to.
     * @param id The handler ID returned from subscribe().
     */
    template<typename EventType>
    void unsubscribe(HandlerId id) {
        auto typeIndex = std::type_index(typeid(EventType));
        auto it = handlers_.find(typeIndex);
        if (it != handlers_.end()) {
            auto& vec = it->second;
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [id](const HandlerEntry& entry) { return entry.id == id; }),
                vec.end()
            );
        }
    }

    /**
     * @brief Publish an event to all subscribers.
     * @tparam EventType The event struct type.
     * @param event The event data to publish.
     */
    template<typename EventType>
    void publish(const EventType& event) {
        auto typeIndex = std::type_index(typeid(EventType));
        auto it = handlers_.find(typeIndex);
        if (it != handlers_.end()) {
            for (const auto& entry : it->second) {
                auto* typed = static_cast<TypedHandler<EventType>*>(entry.handler.get());
                typed->invoke(event);
            }
        }
    }

    /**
     * @brief Check if there are any subscribers for an event type.
     * @tparam EventType The event type to check.
     * @return True if at least one handler is subscribed.
     */
    template<typename EventType>
    bool hasSubscribers() const {
        auto typeIndex = std::type_index(typeid(EventType));
        auto it = handlers_.find(typeIndex);
        return it != handlers_.end() && !it->second.empty();
    }

    /**
     * @brief Remove all subscribers for all event types.
     */
    void clear() {
        handlers_.clear();
    }

    /**
     * @brief Get the number of subscribers for an event type.
     * @tparam EventType The event type to check.
     * @return Number of subscribed handlers.
     */
    template<typename EventType>
    std::size_t subscriberCount() const {
        auto typeIndex = std::type_index(typeid(EventType));
        auto it = handlers_.find(typeIndex);
        return it != handlers_.end() ? it->second.size() : 0;
    }

private:
    // Type-erased handler base class
    struct HandlerBase {
        virtual ~HandlerBase() = default;
    };

    // Typed handler that stores the actual std::function
    template<typename EventType>
    struct TypedHandler : HandlerBase {
        std::function<void(const EventType&)> handler;

        explicit TypedHandler(std::function<void(const EventType&)> h)
            : handler(std::move(h)) {}

        void invoke(const EventType& event) {
            if (handler) {
                handler(event);
            }
        }
    };

    // Entry storing handler ID and type-erased handler
    struct HandlerEntry {
        HandlerId id;
        std::shared_ptr<HandlerBase> handler;

        HandlerEntry(HandlerId i, std::shared_ptr<HandlerBase> h)
            : id(i), handler(std::move(h)) {}
    };

    std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers_;
    HandlerId nextHandlerId_;
};

} // namespace MicropolisEngine

#endif // MICROPOLIS_EVENT_BUS_H
