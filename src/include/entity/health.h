#pragma once

class HealthComponent {
public:
    HealthComponent(float initialHealth = 100.0f): health_(initialHealth), isDead_(false), isDeadPrev_(false) {}

    void doDamage(float damage) {
        if (health_ - damage <= 0.0f) {
            health_ = 0.0f;
            isDead_ = true;
        } else {
            health_ -= damage;
        }
    }

    void kill() {
        health_ = 0.0f;
        isDead_ = true;
    }

    bool isAlive() const {
        return !isDead_;
    }

    bool isDead() const {
        return isDead_;
    }

    bool isJustDead() const {
        return isDead_ && !isDeadPrev_;
    }

    void update() {
        isDeadPrev_ = isDead_;
    }

private:
    float health_ { 100.0f };
    bool isDead_ { false };
    bool isDeadPrev_ { false };
};
