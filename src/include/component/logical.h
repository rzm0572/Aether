#pragma once

class GameObject;

class LogicalComponent {
public:
    virtual ~LogicalComponent() = default;
    virtual void update(float dt, GameObject& obj) = 0;
};
