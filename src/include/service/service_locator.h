#pragma once

// Service Locator class template
// This class template provides a way to access a single instance of a service globally.
// Usage:
// 1. Define a service class, e.g. ShaderManager in a GameEngine instance
// 2. After the service is created and initialized, call ServiceLocator<Service>::provide(service_instance)
// 3. In other parts of the code, call ServiceLocator<Service>::get() to get the single instance of the service
// 4. When the service is no longer needed, call ServiceLocator<Service>::provide(nullptr) to release the instance
template<typename Service>
class ServiceLocator {
public:
    // Provide a service instance to global access
    static void provide(Service* service) {
        instance = service;
    }
    
    // Get the single instance of the service
    static Service* get() {
        return instance;
    }

private:
    inline static Service* instance = nullptr;
};
