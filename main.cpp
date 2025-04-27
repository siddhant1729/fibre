#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float degToRad(float degrees) {
    return degrees * static_cast<float>(M_PI) / 180.f;
}

float radToDeg(float radians) {
    return radians * 180.f / static_cast<float>(M_PI);
}

struct LightRay {
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    bool exited = false;

    float currentIncidenceAngle = 0.f;
    float currentRefractionAngle = 0.f;
    bool hasUpdatedAngles = false;

    LightRay(sf::Vector2f position, sf::Vector2f velocity)
        : velocity(velocity) {
        shape.setSize(sf::Vector2f(50.f, 2.f));
        shape.setFillColor(sf::Color::Cyan);
        shape.setOrigin(0.f, shape.getSize().y / 2.f);
        shape.setPosition(position);

        float angle = radToDeg(std::atan2(velocity.y, velocity.x));
        shape.setRotation(angle);
    }

    void update(float fiberTop, float fiberBottom, float n1, float n2) {
        if (exited) {
            shape.move(velocity);
            return;
        }

        shape.move(velocity);

        float angleRad = degToRad(shape.getRotation());
        sf::Vector2f frontOffset(std::cos(angleRad) * shape.getSize().x,
                                 std::sin(angleRad) * shape.getSize().x);
        sf::Vector2f frontPoint = shape.getPosition() + frontOffset;

        if (frontPoint.y <= fiberTop || frontPoint.y >= fiberBottom) {
            sf::Vector2f normal = (frontPoint.y <= fiberTop) ? sf::Vector2f(0.f, 1.f) : sf::Vector2f(0.f, -1.f);
            sf::Vector2f vNorm = velocity / std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

            float cosTheta1 = vNorm.x * normal.x + vNorm.y * normal.y;
            float theta1 = std::acos(cosTheta1);
            float sinTheta2 = (n1 / n2) * std::sin(theta1);

            currentIncidenceAngle = radToDeg(theta1);
            hasUpdatedAngles = true;

            if (sinTheta2 > 1.f) {
                // Total Internal Reflection
                velocity.y = -velocity.y;
                currentRefractionAngle = -1.f;
            } else {
                float theta2 = std::asin(sinTheta2);
                currentRefractionAngle = radToDeg(theta2);

                float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
                float sign = (frontPoint.y <= fiberTop) ? -1.f : 1.f;
                velocity.y = sign * std::sin(theta2) * speed;
                velocity.x = std::cos(theta2) * speed;
                exited = true;
                shape.setFillColor(sf::Color::Red);
            }

            float newAngle = std::atan2(velocity.y, velocity.x);
            shape.setRotation(radToDeg(newAngle));
        }
    }

    bool isOutOfBounds() const {
        sf::Vector2f pos = shape.getPosition();
        return pos.x > 800 || pos.y < 0 || pos.y > 600;
    }
};

int main() {
    constexpr int windowWidth = 800;
    constexpr int windowHeight = 600;
    constexpr float fiberTop = 200.f;
    constexpr float fiberBottom = 400.f;

    float angleDeg, n1, n2;
    std::cout << "Enter incident angle in degrees (e.g., 60): ";
    std::cin >> angleDeg;
    std::cout << "Enter core refractive index (n1): ";
    std::cin >> n1;
    std::cout << "Enter cladding refractive index (n2): ";
    std::cin >> n2;

    float angleRad = degToRad(angleDeg);
    sf::Vector2f initialVelocity(std::cos(angleRad) * 1.2f, std::sin(angleRad) * 1.2f); // Increased velocity
    sf::Vector2f initialPosition(100.f, 300.f);

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Optical Fibre - TIR & Refraction");

    // Load font
    sf::Font font;
    if (!font.loadFromFile("Arimo-Italic-VariableFont_wght.ttf")) {
        std::cerr << "Error loading font. Make sure 'Arimo-Italic-VariableFont_wght.ttf' is in the directory.\n";
        return -1;
    }

    sf::Text angleText;
    angleText.setFont(font);
    angleText.setCharacterSize(16);
    angleText.setFillColor(sf::Color::White);
    angleText.setStyle(sf::Text::Italic);
    angleText.setPosition(10.f, 10.f);

    sf::Clock clock;
    float updateInterval = 0.02f;

    std::vector<LightRay> rays;
    rays.emplace_back(initialPosition, initialVelocity);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        if (clock.getElapsedTime().asSeconds() >= updateInterval) {
            for (auto& ray : rays)
                ray.update(fiberTop, fiberBottom, n1, n2);

            rays.erase(std::remove_if(rays.begin(), rays.end(),
                                      [](const LightRay& r) { return r.isOutOfBounds(); }),
                       rays.end());

            clock.restart();
        }

        if (!rays.empty() && rays[0].hasUpdatedAngles) {
            std::string text = "Incidence angle: " + std::to_string(rays[0].currentIncidenceAngle) + "°\n";
            if (rays[0].currentRefractionAngle < 0.f)
                text += "Refraction: TIR";
            else
                text += "Refraction angle: " + std::to_string(rays[0].currentRefractionAngle) + "°";
            angleText.setString(text);
        }

        window.clear(sf::Color::Black);

        // Draw fiber bounds
        sf::RectangleShape topLine(sf::Vector2f(windowWidth, 2));
        topLine.setPosition(0, fiberTop);
        topLine.setFillColor(sf::Color(150, 150, 150));

        sf::RectangleShape bottomLine(sf::Vector2f(windowWidth, 2));
        bottomLine.setPosition(0, fiberBottom);
        bottomLine.setFillColor(sf::Color(150, 150, 150));

        window.draw(topLine);
        window.draw(bottomLine);

        for (const auto& ray : rays)
            window.draw(ray.shape);

        window.draw(angleText);
        window.display();
    }

    return 0;
}

