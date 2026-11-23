#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <random>

const float PI = 3.14159265f;
const float GRAVITY = 680.0f;
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float MAX_CLUSTER_SPEED = 350.0f;

struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float life;
    float maxLife;
    float size;
    sf::Color startColor;
    sf::Color endColor;
};

struct Obstacle {
    float x;
    float width = 16.0f;
    float height = 48.0f;
    bool hit = false;
    bool passed = false;
};

struct GameAudio {
    sf::Music bgMusic;
    sf::SoundBuffer accelBuf;
    sf::SoundBuffer thrusterBuf;
    sf::SoundBuffer crashBuf;
    sf::SoundBuffer jumpBuf;

    sf::Sound accelSound{accelBuf};
    sf::Sound thrusterSound{thrusterBuf};
    sf::Sound crashSound{crashBuf};
    sf::Sound jumpSound{jumpBuf};

    bool hasBg = false;
    bool hasAccel = false;
    bool hasThruster = false;
    bool hasCrash = false;
    bool hasJump = false;

    void loadAll() {
        if (bgMusic.openFromFile("background.ogg")) {
            bgMusic.setLooping(true);
            bgMusic.setVolume(30.0f);
            bgMusic.play();
            hasBg = true;
        }
        if (accelBuf.loadFromFile("accel.wav")) {
            accelSound.setBuffer(accelBuf);
            accelSound.setLooping(true);
            accelSound.setVolume(45.0f);
            hasAccel = true;
        }
        if (thrusterBuf.loadFromFile("thruster.wav")) {
            thrusterSound.setBuffer(thrusterBuf);
            thrusterSound.setLooping(true);
            thrusterSound.setVolume(60.0f);
            hasThruster = true;
        }
        if (crashBuf.loadFromFile("crash.wav")) {
            crashSound.setBuffer(crashBuf);
            crashSound.setVolume(80.0f);
            hasCrash = true;
        }
        if (jumpBuf.loadFromFile("jump.wav")) {
            jumpSound.setBuffer(jumpBuf);
            jumpSound.setVolume(65.0f);
            hasJump = true;
        }
    }
};

float smoothHill(float x, float centerX, float peakHeight, float width) {
    float dx = (x - centerX) / width;
    return peakHeight * std::exp(-(dx * dx));
}

float getTerrainHeight(float x) {
    float base = 540.0f;
    float gentleWaves = std::sin(x * 0.0016f) * 28.0f + std::cos(x * 0.0038f + 0.5f) * 10.0f;

    float tallHill1 = smoothHill(x, 2600.0f, 185.0f, 360.0f);
    float tallHill2 = smoothHill(x, 7200.0f, 220.0f, 400.0f);
    float tallHill3 = smoothHill(x, 12400.0f, 170.0f, 340.0f);

    return base + gentleWaves - (tallHill1 + tallHill2 + tallHill3);
}

float getTerrainAngle(float x) {
    float delta = 4.0f;
    float y1 = getTerrainHeight(x - delta);
    float y2 = getTerrainHeight(x + delta);
    float rawAngle = std::atan2(y2 - y1, 2.0f * delta);
    return std::max(-0.65f, std::min(0.65f, rawAngle));
}

sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    return sf::Color(
        static_cast<std::uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<std::uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<std::uint8_t>(a.b + (b.b - a.b) * t),
        static_cast<std::uint8_t>(a.a + (b.a - a.a) * t)
    );
}

void drawPineTree(sf::RenderWindow& window, float x, float baseY, float scale, bool isNightMode) {
    sf::RectangleShape trunk({8.0f * scale, 22.0f * scale});
    trunk.setOrigin({4.0f * scale, 22.0f * scale});
    trunk.setPosition({x, baseY});
    trunk.setFillColor(isNightMode ? sf::Color(48, 32, 24) : sf::Color(92, 58, 38));
    window.draw(trunk);

    sf::Color foliage1 = isNightMode ? sf::Color(16, 68, 48) : sf::Color(34, 139, 68);
    sf::Color foliage2 = isNightMode ? sf::Color(22, 86, 60) : sf::Color(46, 162, 82);

    for (int layer = 0; layer < 2; ++layer) {
        float yShift = (20.0f + layer * 16.0f) * scale;
        float w = (34.0f - layer * 6.0f) * scale;
        float h = (32.0f - layer * 4.0f) * scale;

        sf::ConvexShape cone(3);
        cone.setPoint(0, {x, baseY - yShift - h});
        cone.setPoint(1, {x - w * 0.5f, baseY - yShift});
        cone.setPoint(2, {x + w * 0.5f, baseY - yShift});
        cone.setFillColor(layer == 0 ? foliage1 : foliage2);
        window.draw(cone);
    }
}

void drawHouse(sf::RenderWindow& window, float x, float baseY, sf::Color wallColor, sf::Color roofColor, bool isNightMode) {
    float w = 78.0f;
    float h = 52.0f;

    sf::RectangleShape walls({w, h});
    walls.setPosition({x, baseY - h});
    walls.setFillColor(wallColor);
    window.draw(walls);

    sf::ConvexShape roof(3);
    roof.setPoint(0, {x + w * 0.5f, baseY - h - 28.0f});
    roof.setPoint(1, {x - 8.0f, baseY - h});
    roof.setPoint(2, {x + w + 8.0f, baseY - h});
    roof.setFillColor(roofColor);
    window.draw(roof);

    sf::RectangleShape door({14.0f, 24.0f});
    door.setPosition({x + 14.0f, baseY - 24.0f});
    door.setFillColor(isNightMode ? sf::Color(40, 28, 22) : sf::Color(95, 60, 40));
    window.draw(door);

    sf::Color winColor = isNightMode ? sf::Color(255, 215, 110) : sf::Color(165, 220, 250);
    for (int i = 0; i < 2; ++i) {
        sf::RectangleShape win({15.0f, 14.0f});
        win.setPosition({x + 38.0f + i * 20.0f, baseY - 38.0f});
        win.setFillColor(winColor);
        window.draw(win);
    }
}

void drawHospital(sf::RenderWindow& window, float x, float baseY, bool isNightMode) {
    float w = 135.0f;
    float h = 88.0f;

    sf::RectangleShape building({w, h});
    building.setPosition({x, baseY - h});
    building.setFillColor(isNightMode ? sf::Color(205, 215, 228) : sf::Color(245, 248, 252));
    building.setOutlineThickness(1.5f);
    building.setOutlineColor(isNightMode ? sf::Color(90, 110, 135) : sf::Color(180, 195, 210));
    window.draw(building);

    sf::RectangleShape trim({w + 8.0f, 8.0f});
    trim.setPosition({x - 4.0f, baseY - h - 8.0f});
    trim.setFillColor(sf::Color(210, 45, 55));
    window.draw(trim);

    float cx = x + w * 0.5f;
    float cy = baseY - h + 22.0f;
    sf::RectangleShape crossH({22.0f, 7.0f});
    crossH.setOrigin({11.0f, 3.5f});
    crossH.setPosition({cx, cy});
    crossH.setFillColor(sf::Color(225, 35, 45));
    window.draw(crossH);

    sf::RectangleShape crossV({7.0f, 22.0f});
    crossV.setOrigin({3.5f, 11.0f});
    crossV.setPosition({cx, cy});
    crossV.setFillColor(sf::Color(225, 35, 45));
    window.draw(crossV);

    sf::Color winColor = isNightMode ? sf::Color(180, 240, 255) : sf::Color(120, 185, 225);
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 4; ++col) {
            sf::RectangleShape win({16.0f, 13.0f});
            win.setPosition({x + 16.0f + col * 28.0f, baseY - 46.0f + row * 22.0f});
            win.setFillColor(winColor);
            window.draw(win);
        }
    }
}

void drawSideScenery(sf::RenderWindow& window, float camCenterX, bool isNightMode) {
    float parallaxFactor = 0.35f;
    float worldScroll = camCenterX * parallaxFactor;
    float period = 5500.0f;

    float horizonY = 535.0f;
    sf::Color groundStripColor = isNightMode ? sf::Color(14, 24, 34) : sf::Color(115, 185, 110);
    sf::RectangleShape farGround({(float)WINDOW_WIDTH, 200.0f});
    farGround.setPosition({0.0f, horizonY});
    farGround.setFillColor(groundStripColor);
    window.draw(farGround);

    int firstTile = static_cast<int>(std::floor((worldScroll - 400.0f) / period));
    int lastTile = static_cast<int>(std::ceil((worldScroll + WINDOW_WIDTH + 400.0f) / period));

    for (int t = firstTile; t <= lastTile; ++t) {
        float baseTileX = t * period - worldScroll;

        for (int i = 0; i < 8; ++i) {
            float tx = baseTileX + 250.0f + i * 52.0f;
            float sc = 0.78f + ((std::abs(i * 19 + t * 7) % 4) * 0.12f);
            drawPineTree(window, tx, horizonY + 8.0f, sc, isNightMode);
        }

        drawHouse(
            window,
            baseTileX + 1450.0f,
            horizonY + 6.0f,
            isNightMode ? sf::Color(145, 115, 85) : sf::Color(235, 198, 145),
            isNightMode ? sf::Color(115, 42, 38) : sf::Color(175, 68, 52),
            isNightMode
        );
        drawHouse(
            window,
            baseTileX + 1580.0f,
            horizonY + 6.0f,
            isNightMode ? sf::Color(110, 130, 145) : sf::Color(215, 225, 232),
            isNightMode ? sf::Color(55, 75, 105) : sf::Color(95, 125, 165),
            isNightMode
        );

        drawHospital(window, baseTileX + 3100.0f, horizonY + 6.0f, isNightMode);

        for (int i = 0; i < 5; ++i) {
            float tx = baseTileX + 4200.0f + i * 85.0f;
            float sc = 0.8f + ((std::abs(i * 23 + t * 11) % 3) * 0.12f);
            drawPineTree(window, tx, horizonY + 8.0f, sc, isNightMode);
        }
    }
}

class TeslaRoadster {
public:
    sf::Vector2f pos;
    sf::Vector2f vel;
    float angle;
    float angularVel;
    float fuel;
    float suspensionOffset;
    float suspensionVel;
    float wheelRotation;
    char gearCode; // 'R', 'N', or 'D'
    int crashCount;
    int passedCount;
    float maxDistance;

    bool isGrounded;
    bool suctionActive;
    bool thrustersFiring;
    bool autoLanding;

    TeslaRoadster(float startX) {
        pos = {startX, getTerrainHeight(startX) - 25.0f};
        vel = {0.0f, 0.0f};
        angle = 0.0f;
        angularVel = 0.0f;
        fuel = 100.0f;
        suspensionOffset = 0.0f;
        suspensionVel = 0.0f;
        wheelRotation = 0.0f;
        gearCode = 'N';
        crashCount = 0;
        passedCount = 0;
        maxDistance = 0.0f;
        isGrounded = true;
        suctionActive = true;
        thrustersFiring = false;
        autoLanding = false;
    }

    void update(float dt, std::vector<Particle>& particles, std::vector<Obstacle>& obstacles, float startX, GameAudio& audio) {
        float groundY = getTerrainHeight(pos.x);
        float groundAngle = getTerrainAngle(pos.x);
        float carClearance = 22.0f;
        float distToGround = (groundY - carClearance) - pos.y;

        thrustersFiring = false;
        autoLanding = false;

        float driveAccel = 1150.0f;
        float maxSpeed = 1030.0f;
        bool accelerating = false;

        maxDistance = std::max(maxDistance, pos.x - startX);

        bool rightHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
        bool leftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);

        if (rightHeld) {
            gearCode = 'D';
            accelerating = true;
            if (isGrounded) {
                vel.x += std::cos(groundAngle) * driveAccel * dt;
                vel.y += std::sin(groundAngle) * driveAccel * dt;
            } else {
                vel.x += driveAccel * 0.65f * dt;
                angularVel += 1.5f * dt;
            }
        }
        if (leftHeld) {
            gearCode = 'R';
            accelerating = true;
            if (isGrounded) {
                vel.x -= std::cos(groundAngle) * driveAccel * 0.65f * dt;
                vel.y -= std::sin(groundAngle) * driveAccel * 0.65f * dt;
            } else {
                vel.x -= driveAccel * 0.65f * dt;
                angularVel -= 1.5f * dt;
            }
        }

        if (isGrounded && !accelerating) {
            vel.x *= std::pow(0.001f, dt);
            vel.y *= std::pow(0.001f, dt);
            if (std::abs(vel.x) < 2.0f) vel.x = 0.0f;
            if (std::abs(vel.y) < 2.0f) vel.y = 0.0f;
            if (std::abs(vel.x) < 1.0f) gearCode = 'N';
        }

        if (audio.hasAccel) {
            if (accelerating) {
                if (audio.accelSound.getStatus() != sf::Sound::Status::Playing) {
                    audio.accelSound.play();
                }
            } else if (audio.accelSound.getStatus() == sf::Sound::Status::Playing) {
                audio.accelSound.stop();
            }
        }

        vel.x = std::max(-maxSpeed, std::min(maxSpeed, vel.x));

        if ((sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) && isGrounded) {
            vel.y = -390.0f;
            isGrounded = false;
            suspensionVel = 120.0f;
            if (audio.hasJump) {
                audio.jumpSound.play();
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && fuel > 0.0f) {
            thrustersFiring = true;
            isGrounded = false;
            fuel = std::max(0.0f, fuel - 24.0f * dt);

            float thrustPower = 1150.0f;
            vel.y -= thrustPower * dt;

            if (gearCode == 'D') {
                vel.x += 520.0f * dt;
            }

            angle += (0.0f - angle) * 5.0f * dt;
            angularVel *= 0.85f;

            spawnThrusterParticles(particles);
        } else {
            float rechargeRate = isGrounded ? 28.0f : 10.0f;
            fuel = std::min(100.0f, fuel + rechargeRate * dt);
        }

        if (!isGrounded && distToGround < 140.0f && vel.y > 120.0f && fuel > 5.0f) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
                autoLanding = true;
                thrustersFiring = true;
                vel.y -= 980.0f * dt;
                fuel = std::max(0.0f, fuel - 15.0f * dt);
                angle += (groundAngle - angle) * 6.0f * dt;
                spawnThrusterParticles(particles);
            }
        }

        if (audio.hasThruster) {
            if (thrustersFiring) {
                if (audio.thrusterSound.getStatus() != sf::Sound::Status::Playing) {
                    audio.thrusterSound.play();
                }
            } else if (audio.thrusterSound.getStatus() == sf::Sound::Status::Playing) {
                audio.thrusterSound.stop();
            }
        }

        vel.y += GRAVITY * dt;

        if (suctionActive && distToGround < 38.0f && vel.y >= -60.0f) {
            float suctionForce = 1250.0f;
            vel.x += -std::sin(groundAngle) * suctionForce * dt;
            vel.y += std::cos(groundAngle) * suctionForce * dt;

            if (std::abs(vel.x) > 40.0f && (rand() % 3 == 0)) {
                Particle p;
                p.pos = {pos.x + (rand() % 60 - 30), groundY - 2.0f};
                p.vel = {-vel.x * 0.1f, -20.0f - (rand() % 20)};
                p.life = 0.0f;
                p.maxLife = 0.25f;
                p.size = 2.5f;
                p.startColor = sf::Color(0, 240, 255, 180);
                p.endColor = sf::Color(0, 120, 255, 0);
                particles.push_back(p);
            }
        }

        pos += vel * dt;

        for (auto& obs : obstacles) {
            float obsY = getTerrainHeight(obs.x) - obs.height / 2.0f;
            if (std::abs(pos.x - obs.x) < 30.0f && std::abs(pos.y - obsY) < 30.0f) {
                if (!obs.hit) {
                    crashCount++;
                    obs.hit = true;
                    if (audio.hasCrash) {
                        audio.crashSound.play();
                    }
                }
                if (pos.x < obs.x) {
                    pos.x = obs.x - 30.0f;
                    vel.x = -std::abs(vel.x) * 0.3f;
                } else {
                    pos.x = obs.x + 30.0f;
                    vel.x = std::abs(vel.x) * 0.3f;
                }
            } else if (!obs.hit && !obs.passed && pos.x > obs.x + 32.0f) {
                obs.passed = true;
                passedCount++;
            }
        }

        groundY = getTerrainHeight(pos.x);
        groundAngle = getTerrainAngle(pos.x);

        if (pos.y >= groundY - carClearance) {
            if (!isGrounded && vel.y > 100.0f) {
                suspensionOffset = std::min(10.0f, vel.y * 0.025f);
            }
            pos.y = groundY - carClearance;
            isGrounded = true;

            float speed = std::hypot(vel.x, vel.y) * (vel.x >= 0 ? 1.0f : -1.0f);
            vel.x = std::cos(groundAngle) * speed;
            vel.y = std::sin(groundAngle) * speed;

            angle += (groundAngle - angle) * std::min(1.0f, 16.0f * dt);
            angularVel = 0.0f;
        } else {
            isGrounded = false;
            angle += angularVel * dt;
            angularVel *= 0.96f;
        }

        float springForce = -120.0f * suspensionOffset - 12.0f * suspensionVel;
        suspensionVel += springForce * dt;
        suspensionOffset += suspensionVel * dt;

        wheelRotation += vel.x * dt * 2.5f;
    }

    void spawnThrusterParticles(std::vector<Particle>& particles) {
        for (int i = 0; i < 3; ++i) {
            Particle p;
            float offsetX = (i == 0) ? -32.0f : ((i == 1) ? -12.0f : 16.0f);
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            p.pos.x = pos.x + offsetX * cosA - 12.0f * sinA;
            p.pos.y = pos.y + offsetX * sinA + 12.0f * cosA;

            float spread = ((rand() % 100) / 100.0f - 0.5f) * 90.0f;
            p.vel.x = vel.x * 0.3f - sinA * 320.0f + spread;
            p.vel.y = vel.y * 0.3f + cosA * 320.0f + (rand() % 60);

            p.life = 0.0f;
            p.maxLife = 0.22f + (rand() % 10) * 0.015f;
            p.size = 5.0f + (rand() % 4);
            p.startColor = sf::Color(120, 230, 255, 255);
            p.endColor = sf::Color(255, 110, 20, 0);
            particles.push_back(p);
        }
    }

    void draw(sf::RenderWindow& window) {
        sf::Transform transform;
        transform.translate({pos.x, pos.y + suspensionOffset});
        transform.rotate(sf::degrees(angle * 180.0f / PI));

        if (suctionActive) {
            sf::ConvexShape glow(4);
            glow.setPoint(0, {-42.0f, 12.0f});
            glow.setPoint(1, {42.0f, 12.0f});
            glow.setPoint(2, {50.0f, 24.0f});
            glow.setPoint(3, {-50.0f, 24.0f});
            glow.setFillColor(sf::Color(0, 220, 255, 55));
            window.draw(glow, transform);
        }

        sf::ConvexShape body(7);
        body.setPoint(0, {-46.0f, 8.0f});
        body.setPoint(1, {-44.0f, -6.0f});
        body.setPoint(2, {-18.0f, -16.0f});
        body.setPoint(3, {8.0f, -15.0f});
        body.setPoint(4, {36.0f, -4.0f});
        body.setPoint(5, {48.0f, 3.0f});
        body.setPoint(6, {45.0f, 10.0f});
        body.setFillColor(sf::Color(225, 22, 38));
        body.setOutlineThickness(1.5f);
        body.setOutlineColor(sf::Color(255, 90, 100));
        window.draw(body, transform);

        sf::ConvexShape glass(4);
        glass.setPoint(0, {-16.0f, -14.0f});
        glass.setPoint(1, {7.0f, -13.5f});
        glass.setPoint(2, {27.0f, -5.0f});
        glass.setPoint(3, {-22.0f, -5.0f});
        glass.setFillColor(sf::Color(25, 35, 50));
        window.draw(glass, transform);

        sf::RectangleShape rearThruster({10.0f, 6.0f});
        rearThruster.setPosition({-48.0f, 1.0f});
        rearThruster.setFillColor(sf::Color(50, 55, 65));
        window.draw(rearThruster, transform);

        sf::CircleShape headlight(3.0f);
        headlight.setPosition({42.0f, -1.0f});
        headlight.setFillColor(sf::Color(220, 250, 255));
        window.draw(headlight, transform);

        sf::RectangleShape taillight({4.0f, 3.0f});
        taillight.setPosition({-46.5f, -4.0f});
        taillight.setFillColor(sf::Color(255, 30, 30));
        window.draw(taillight, transform);

        drawWheel(window, -28.0f, 11.0f);
        drawWheel(window, 28.0f, 11.0f);
    }

    void drawWheel(sf::RenderWindow& window, float offsetX, float offsetY) {
        sf::Transform wTransform;
        wTransform.translate({pos.x, pos.y});
        wTransform.rotate(sf::degrees(angle * 180.0f / PI));
        wTransform.translate({offsetX, offsetY});
        wTransform.rotate(sf::degrees(wheelRotation));

        sf::CircleShape tire(11.5f);
        tire.setOrigin({11.5f, 11.5f});
        tire.setFillColor(sf::Color(24, 24, 28));
        tire.setOutlineThickness(2.0f);
        tire.setOutlineColor(sf::Color(45, 45, 52));
        window.draw(tire, wTransform);

        sf::CircleShape rim(6.5f);
        rim.setOrigin({6.5f, 6.5f});
        rim.setFillColor(sf::Color(180, 190, 205));
        window.draw(rim, wTransform);

        sf::RectangleShape spoke1({12.0f, 2.0f});
        spoke1.setOrigin({6.0f, 1.0f});
        spoke1.setFillColor(sf::Color(40, 45, 55));
        window.draw(spoke1, wTransform);

        sf::RectangleShape spoke2({2.0f, 12.0f});
        spoke2.setOrigin({1.0f, 6.0f});
        spoke2.setFillColor(sf::Color(40, 45, 55));
        window.draw(spoke2, wTransform);
    }
};

void drawDigitalCluster(
    sf::RenderWindow& window,
    const sf::Font& font,
    bool fontLoaded,
    float smoothSpeed,
    char gearCode,
    float fuel,
    float distanceMeters,
    bool thrustersFiring
) {
    // Compact bottom-right instrument cluster without stray blue bar
    float panelW = 248.0f;
    float panelH = 172.0f;
    float px = (float)WINDOW_WIDTH - panelW - 18.0f;
    float py = (float)WINDOW_HEIGHT - panelH - 16.0f;

    sf::RectangleShape pod({panelW, panelH});
    pod.setPosition({px, py});
    pod.setFillColor(sf::Color(7, 11, 20, 232));
    pod.setOutlineThickness(1.0f);
    pod.setOutlineColor(sf::Color(40, 85, 135, 125));
    window.draw(pod);

    float cx = px + panelW * 0.5f;
    float cy = py + 92.0f;
    float outerR = 64.0f;

    // Subtle inner depth circle & clean perspective road lines
    sf::CircleShape innerDial(outerR - 6.0f);
    innerDial.setOrigin({outerR - 6.0f, outerR - 6.0f});
    innerDial.setPosition({cx, cy});
    innerDial.setFillColor(sf::Color(10, 16, 30, 220));
    window.draw(innerDial);

    sf::ConvexShape leftRoadEdge(3);
    leftRoadEdge.setPoint(0, {cx - 3.0f, cy + 12.0f});
    leftRoadEdge.setPoint(1, {cx - 1.0f, cy + 12.0f});
    leftRoadEdge.setPoint(2, {cx - 36.0f, cy + 48.0f});
    leftRoadEdge.setFillColor(sf::Color(165, 205, 240, 130));
    window.draw(leftRoadEdge);

    sf::ConvexShape rightRoadEdge(3);
    rightRoadEdge.setPoint(0, {cx + 1.0f, cy + 12.0f});
    rightRoadEdge.setPoint(1, {cx + 3.0f, cy + 12.0f});
    rightRoadEdge.setPoint(2, {cx + 36.0f, cy + 48.0f});
    rightRoadEdge.setFillColor(sf::Color(165, 205, 240, 130));
    window.draw(rightRoadEdge);

    // Mini car silhouette
    sf::ConvexShape miniCar(6);
    miniCar.setPoint(0, {cx - 11.0f, cy + 44.0f});
    miniCar.setPoint(1, {cx - 9.5f, cy + 31.0f});
    miniCar.setPoint(2, {cx - 6.0f, cy + 25.0f});
    miniCar.setPoint(3, {cx + 6.0f, cy + 25.0f});
    miniCar.setPoint(4, {cx + 9.5f, cy + 31.0f});
    miniCar.setPoint(5, {cx + 11.0f, cy + 44.0f});
    miniCar.setFillColor(sf::Color(225, 232, 242));
    window.draw(miniCar);

    sf::RectangleShape rearLightBar({15.0f, 2.0f});
    rearLightBar.setOrigin({7.5f, 1.0f});
    rearLightBar.setPosition({cx, cy + 33.0f});
    rearLightBar.setFillColor(thrustersFiring ? sf::Color(0, 220, 255) : sf::Color(255, 60, 60));
    window.draw(rearLightBar);

    // 0 to 350 km/h sweep
    float startDeg = -215.0f;
    float sweepDeg = 250.0f;

    for (int v = 0; v <= static_cast<int>(MAX_CLUSTER_SPEED); v += 25) {
        float frac = static_cast<float>(v) / MAX_CLUSTER_SPEED;
        float deg = startDeg + frac * sweepDeg;
        float rad = deg * PI / 180.0f;

        bool isMajor = (v % 50 == 0);
        float innerR = isMajor ? (outerR - 8.0f) : (outerR - 4.5f);

        sf::Vector2f pInner(cx + std::cos(rad) * innerR, cy + std::sin(rad) * innerR);
        sf::Vector2f pOuter(cx + std::cos(rad) * outerR, cy + std::sin(rad) * outerR);

        sf::Vertex tickLine[2];
        sf::Color tickCol = (v >= 300) ? sf::Color(255, 75, 75) : (isMajor ? sf::Color(220, 235, 250) : sf::Color(95, 120, 150));
        tickLine[0] = sf::Vertex(pInner, tickCol);
        tickLine[1] = sf::Vertex(pOuter, tickCol);
        window.draw(tickLine, 2, sf::PrimitiveType::Lines);

        if (fontLoaded && (v == 0 || v == 100 || v == 200 || v == 300 || v == 350)) {
            float labelR = outerR - 17.0f;
            sf::Text numText(font, std::to_string(v), 9);
            sf::FloatRect bounds = numText.getLocalBounds();
            numText.setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});
            numText.setPosition({cx + std::cos(rad) * labelR, cy + std::sin(rad) * labelR});
            numText.setFillColor(v >= 300 ? sf::Color(255, 95, 95) : sf::Color(195, 215, 235));
            window.draw(numText);
        }
    }

    // Active arc
    float clampedSpeed = std::max(0.0f, std::min(MAX_CLUSTER_SPEED, smoothSpeed));
    int arcSegments = 36;
    float activeFrac = clampedSpeed / MAX_CLUSTER_SPEED;
    int activeSteps = std::max(1, static_cast<int>(arcSegments * activeFrac));

    for (int i = 0; i < activeSteps; ++i) {
        float t0 = static_cast<float>(i) / arcSegments * activeFrac;
        float t1 = static_cast<float>(i + 1) / arcSegments * activeFrac;
        float a0 = (startDeg + t0 * sweepDeg) * PI / 180.0f;
        float a1 = (startDeg + t1 * sweepDeg) * PI / 180.0f;

        sf::Vertex seg[2];
        sf::Color arcCol = (clampedSpeed > 270.0f) ? sf::Color(255, 65, 65) : sf::Color(0, 210, 255);
        seg[0] = sf::Vertex({cx + std::cos(a0) * (outerR + 2.0f), cy + std::sin(a0) * (outerR + 2.0f)}, arcCol);
        seg[1] = sf::Vertex({cx + std::cos(a1) * (outerR + 2.0f), cy + std::sin(a1) * (outerR + 2.0f)}, arcCol);
        window.draw(seg, 2, sf::PrimitiveType::Lines);
    }

    // Needle
    float needleDeg = startDeg + (clampedSpeed / MAX_CLUSTER_SPEED) * sweepDeg;
    sf::RectangleShape needle({outerR - 6.0f, 2.5f});
    needle.setOrigin({0.0f, 1.25f});
    needle.setPosition({cx, cy});
    needle.setFillColor(sf::Color(255, 55, 45));
    needle.setRotation(sf::degrees(needleDeg));
    window.draw(needle);

    sf::CircleShape hub(4.5f);
    hub.setOrigin({4.5f, 4.5f});
    hub.setPosition({cx, cy});
    hub.setFillColor(sf::Color(22, 28, 40));
    hub.setOutlineThickness(1.5f);
    hub.setOutlineColor(sf::Color(255, 75, 65));
    window.draw(hub);

    if (fontLoaded) {
        int dispInt = static_cast<int>(std::round(clampedSpeed));
        sf::Text speedReadout(font, std::to_string(dispInt), 18);
        sf::FloatRect sBounds = speedReadout.getLocalBounds();
        speedReadout.setOrigin({sBounds.size.x * 0.5f, sBounds.size.y * 0.5f});
        speedReadout.setPosition({cx, cy - 15.0f});
        speedReadout.setFillColor(sf::Color::White);
        window.draw(speedReadout);

        sf::Text unitLabel(font, "km/h", 9);
        sf::FloatRect uBounds = unitLabel.getLocalBounds();
        unitLabel.setOrigin({uBounds.size.x * 0.5f, uBounds.size.y * 0.5f});
        unitLabel.setPosition({cx, cy - 29.0f});
        unitLabel.setFillColor(sf::Color(145, 175, 210));
        window.draw(unitLabel);

        // Gear selector R | N | D
        const char gears[3] = {'R', 'N', 'D'};
        for (int g = 0; g < 3; ++g) {
            bool active = (gearCode == gears[g]);
            sf::RectangleShape slot({17.0f, 17.0f});
            slot.setPosition({px + 9.0f + g * 19.0f, py + 9.0f});
            slot.setFillColor(active ? sf::Color(0, 200, 255) : sf::Color(16, 24, 38));
            slot.setOutlineThickness(1.0f);
            slot.setOutlineColor(active ? sf::Color::White : sf::Color(60, 80, 108));
            window.draw(slot);

            sf::Text gTxt(font, std::string(1, gears[g]), 10);
            gTxt.setPosition({px + 14.0f + g * 19.0f, py + 11.0f});
            gTxt.setFillColor(active ? sf::Color(8, 12, 20) : sf::Color(145, 165, 190));
            window.draw(gTxt);
        }

        // Odometer / Distance
        sf::Text odoText(font, std::to_string(static_cast<int>(distanceMeters)) + " km", 10);
        odoText.setPosition({px + 9.0f, py + panelH - 20.0f});
        odoText.setFillColor(sf::Color(170, 190, 215));
        window.draw(odoText);

        // Fuel bar
        float barW = 72.0f;
        float barH = 6.0f;
        float bx = px + panelW - barW - 9.0f;
        float by = py + panelH - 16.0f;

        sf::RectangleShape fBg({barW, barH});
        fBg.setPosition({bx, by});
        fBg.setFillColor(sf::Color(35, 45, 62));
        window.draw(fBg);

        sf::RectangleShape fFill({barW * std::max(0.0f, std::min(1.0f, fuel / 100.0f)), barH});
        fFill.setPosition({bx, by});
        fFill.setFillColor(fuel > 25.0f ? sf::Color(0, 235, 130) : sf::Color(255, 65, 65));
        window.draw(fFill);
    }
}

// Generates varied, non-repeating gaps between 1750 and 3400 units
float nextVariedGap(std::mt19937& rng, float& lastGap) {
    std::uniform_real_distribution<float> dist(1750.0f, 3400.0f);
    float candidate = dist(rng);
    // Avoid repeating nearly identical gap lengths back-to-back
    if (std::abs(candidate - lastGap) < 450.0f) {
        candidate = (candidate > 2575.0f) ? (candidate - 750.0f) : (candidate + 750.0f);
    }
    candidate = std::max(1750.0f, std::min(3400.0f, candidate));
    lastGap = candidate;
    return candidate;
}

int main() {
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;

    sf::RenderWindow window(sf::VideoMode({(unsigned int)WINDOW_WIDTH, (unsigned int)WINDOW_HEIGHT}), "Tesla Roadster 2D - Simulation", sf::State::Windowed, settings);
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf") ||
                      font.openFromFile("C:/Windows/Fonts/segoeui.ttf");

    GameAudio audio;
    audio.loadAll();

    std::random_device rd;
    std::mt19937 rng(rd());
    float lastGap = 2400.0f;

    float startX = 200.0f;
    TeslaRoadster roadster(startX);
    std::vector<Particle> particles;

    // Initial obstacles ahead + dynamically extended forever as player advances
    std::vector<Obstacle> obstacles;
    float nextObsX = 1650.0f;
    for (int i = 0; i < 16; ++i) {
        obstacles.push_back({nextObsX});
        nextObsX += nextVariedGap(rng, lastGap);
    }

    sf::View camera(sf::FloatRect({0, 0}, {(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT}));
    sf::Clock clock;
    bool sKeyWasPressed = false;
    bool nKeyWasPressed = false;
    bool isNightMode = true;
    float smoothSpeed = 0.0f;

    while (window.isOpen()) {
        float dt = std::min(0.05f, clock.restart().asSeconds());

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    roadster = TeslaRoadster(startX);
                    smoothSpeed = 0.0f;
                    particles.clear();
                    obstacles.clear();
                    lastGap = 2400.0f;
                    nextObsX = 1650.0f;
                    for (int i = 0; i < 16; ++i) {
                        obstacles.push_back({nextObsX});
                        nextObsX += nextVariedGap(rng, lastGap);
                    }
                }
            }
        }

        bool sPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
        if (sPressed && !sKeyWasPressed) {
            roadster.suctionActive = !roadster.suctionActive;
        }
        sKeyWasPressed = sPressed;

        bool nPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::N);
        if (nPressed && !nKeyWasPressed) {
            isNightMode = !isNightMode;
        }
        nKeyWasPressed = nPressed;

        roadster.update(dt, particles, obstacles, startX, audio);

        // Keep spawning obstacles infinitely ahead of the roadster with varying non-repeating gaps
        while (nextObsX < roadster.pos.x + 28000.0f) {
            obstacles.push_back({nextObsX});
            nextObsX += nextVariedGap(rng, lastGap);
        }

        // Prune obstacles far behind to keep memory lightweight over long runs
        float pruneX = roadster.pos.x - 2500.0f;
        obstacles.erase(
            std::remove_if(obstacles.begin(), obstacles.end(), [pruneX](const Obstacle& o) {
                return o.x < pruneX;
            }),
            obstacles.end()
        );

        float rawSpeed = std::hypot(roadster.vel.x, roadster.vel.y);
        float targetSpeedKmh = std::min(MAX_CLUSTER_SPEED, rawSpeed * 0.34f);
        float interpRate = (targetSpeedKmh > smoothSpeed) ? 8.5f : 6.0f;
        smoothSpeed += (targetSpeedKmh - smoothSpeed) * std::min(1.0f, interpRate * dt);
        if (targetSpeedKmh < 0.5f && smoothSpeed < 0.8f) {
            smoothSpeed = 0.0f;
        }

        for (size_t i = 0; i < particles.size();) {
            particles[i].life += dt;
            if (particles[i].life >= particles[i].maxLife) {
                particles.erase(particles.begin() + i);
            } else {
                particles[i].pos += particles[i].vel * dt;
                ++i;
            }
        }

        // Responsive camera Y so landing after thruster flight shows car immediately
        float targetCamX = roadster.pos.x + 220.0f + roadster.vel.x * 0.15f;
        float desiredCamY = std::min(400.0f, roadster.pos.y - 35.0f);
        sf::Vector2f currentCam = camera.getCenter();
        float yRate = (desiredCamY > currentCam.y) ? 10.0f : 6.5f;
        camera.setCenter({
            currentCam.x + (targetCamX - currentCam.x) * 7.0f * dt,
            currentCam.y + (desiredCamY - currentCam.y) * std::min(1.0f, yRate * dt)
        });

        sf::Color skyTop = isNightMode ? sf::Color(10, 14, 28) : sf::Color(100, 180, 240);
        sf::Color skyBottom = isNightMode ? sf::Color(28, 42, 72) : sf::Color(200, 230, 255);
        window.clear(skyTop);

        window.setView(window.getDefaultView());

        sf::VertexArray sky(sf::PrimitiveType::TriangleFan, 4);
        sky[0] = sf::Vertex({0, 0}, skyTop);
        sky[1] = sf::Vertex({(float)WINDOW_WIDTH, 0}, skyTop);
        sky[2] = sf::Vertex({(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT}, skyBottom);
        sky[3] = sf::Vertex({0, (float)WINDOW_HEIGHT}, skyBottom);
        window.draw(sky);

        drawSideScenery(window, camera.getCenter().x, isNightMode);

        window.setView(camera);

        float camLeft = camera.getCenter().x - WINDOW_WIDTH * 0.65f;
        float camRight = camera.getCenter().x + WINDOW_WIDTH * 0.65f;
        float step = 5.0f;

        sf::VertexArray terrainFill(sf::PrimitiveType::TriangleStrip);
        sf::VertexArray roadSurface(sf::PrimitiveType::TriangleStrip);

        sf::Color roadFillColor = isNightMode ? sf::Color(25, 32, 48) : sf::Color(140, 150, 165);
        sf::Color neonLineColor = isNightMode ? sf::Color(0, 230, 255) : sf::Color(0, 120, 255);

        for (float x = camLeft; x <= camRight; x += step) {
            float y = getTerrainHeight(x);
            terrainFill.append(sf::Vertex({x, y}, roadFillColor));
            terrainFill.append(sf::Vertex({x, y + 1200.0f}, sf::Color(10, 12, 20)));

            roadSurface.append(sf::Vertex({x, y - 3.0f}, neonLineColor));
            roadSurface.append(sf::Vertex({x, y + 5.0f}, sf::Color(45, 55, 75)));
        }
        window.draw(terrainFill);
        window.draw(roadSurface);

        for (const auto& obs : obstacles) {
            if (obs.x >= camLeft - 100.0f && obs.x <= camRight + 100.0f) {
                float ty = getTerrainHeight(obs.x) - obs.height;
                sf::RectangleShape stick({obs.width, obs.height});
                stick.setOrigin({obs.width / 2.0f, 0.0f});
                stick.setPosition({obs.x, ty});
                stick.setFillColor(obs.hit ? sf::Color(120, 120, 120) : sf::Color(255, 60, 60));
                stick.setOutlineThickness(2.0f);
                stick.setOutlineColor(sf::Color(255, 200, 200));
                window.draw(stick);
            }
        }

        for (const auto& p : particles) {
            float ratio = p.life / p.maxLife;
            sf::CircleShape circle(p.size * (1.0f - ratio * 0.5f));
            circle.setOrigin({circle.getRadius(), circle.getRadius()});
            circle.setPosition(p.pos);
            circle.setFillColor(lerpColor(p.startColor, p.endColor, ratio));
            window.draw(circle);
        }

        roadster.draw(window);

        window.setView(window.getDefaultView());

        int distMeters = static_cast<int>(roadster.maxDistance * 0.2f);

        drawDigitalCluster(
            window,
            font,
            fontLoaded,
            smoothSpeed,
            roadster.gearCode,
            roadster.fuel,
            static_cast<float>(distMeters),
            roadster.thrustersFiring
        );

        // --- TOP-LEFT TELEMETRY HUD ---
        sf::RectangleShape hudPanel({350.0f, 165.0f});
        hudPanel.setPosition({20.0f, 20.0f});
        hudPanel.setFillColor(sf::Color(15, 22, 36, 210));
        hudPanel.setOutlineThickness(1.5f);
        hudPanel.setOutlineColor(sf::Color(0, 220, 255, 160));
        window.draw(hudPanel);

        if (fontLoaded) {
            int altitude = std::max(0, static_cast<int>((getTerrainHeight(roadster.pos.x) - 22.0f - roadster.pos.y) * 0.15f));

            std::stringstream ss;
            ss << "TESLA ROADSTER // TELEMETRY\n"
               << "DISTANCE:   " << distMeters << " M\n"
               << "PASSED:     " << roadster.passedCount << "  |  CRASHED: " << roadster.crashCount << "\n"
               << "ALTITUDE:   " << altitude << " M  [" << (roadster.isGrounded ? "GROUND" : "AIR") << "]\n"
               << "SUCTION:    " << (roadster.suctionActive ? "ACTIVE" : "OFF") << " | [N] " << (isNightMode ? "NIGHT" : "DAY") << "\n";

            sf::Text hudText(font, ss.str(), 14);
            hudText.setPosition({32.0f, 28.0f});
            hudText.setFillColor(sf::Color(230, 245, 255));
            hudText.setLineSpacing(1.2f);
            window.draw(hudText);

            // --- TRACK RADAR (CLAMPED INSIDE BOX) ---
            float mapX = (float)WINDOW_WIDTH - 640.0f;
            float mapY = 20.0f;
            float mapW = 220.0f;
            float mapH = 45.0f;

            sf::RectangleShape mapBox({mapW, mapH});
            mapBox.setPosition({mapX, mapY});
            mapBox.setFillColor(sf::Color(10, 15, 25, 200));
            mapBox.setOutlineThickness(1.5f);
            mapBox.setOutlineColor(sf::Color(0, 230, 255, 150));
            window.draw(mapBox);

            sf::Text mapLabel(font, "TRACK RADAR", 9);
            mapLabel.setPosition({mapX + 8.0f, mapY + 3.0f});
            mapLabel.setFillColor(sf::Color(150, 170, 200));
            window.draw(mapLabel);

            float trackLength = 38000.0f;
            float rawRatio = (roadster.pos.x - startX) / trackLength;
            float clampedRatio = std::max(0.0f, std::min(1.0f, rawRatio));

            float dotMinX = mapX + 10.0f;
            float dotMaxX = mapX + mapW - 10.0f;
            float dotX = dotMinX + clampedRatio * (dotMaxX - dotMinX);

            sf::CircleShape playerDot(4.0f);
            playerDot.setOrigin({4.0f, 4.0f});
            playerDot.setPosition({dotX, mapY + 28.0f});
            playerDot.setFillColor(sf::Color(0, 255, 120));
            window.draw(playerDot);

            sf::RectangleShape ctrlPanel({380.0f, 165.0f});
            ctrlPanel.setPosition({(float)WINDOW_WIDTH - 400.0f, 20.0f});
            ctrlPanel.setFillColor(sf::Color(15, 22, 36, 210));
            ctrlPanel.setOutlineThickness(1.5f);
            ctrlPanel.setOutlineColor(sf::Color(255, 60, 90, 160));
            window.draw(ctrlPanel);

            sf::Text ctrlText(font,
                "FLIGHT & DRIVE CONTROLS\n"
                "[D / Right] : Forward (D) | [A / Left] : Reverse (R)\n"
                "[W / Up]    : Jump Sound & Impulse\n"
                "[SPACE]     : Thrusters + Boost Sound\n"
                "[N] Day/Night  |  [S] Suction  |  [R-Key] Reset",
                13
            );
            ctrlText.setPosition({(float)WINDOW_WIDTH - 388.0f, 28.0f});
            ctrlText.setFillColor(sf::Color(215, 225, 240));
            ctrlText.setLineSpacing(1.2f);
            window.draw(ctrlText);
        }

        window.display();
    }

    return 0;
}