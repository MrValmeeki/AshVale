#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <iostream>
#include "MapManager.h"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <optional>
#include <list>

class Player;
class Enemy;

void updateHitBox(Player& player, Enemy& enemy);
void updateHealing(Player& player);

// SFX class to handle all audio-related functionality
class SFX {
private:
    sf::Music backgroundMusic;
    sf::Music criticalThemeMusic;
    bool criticalMusicPlaying = false;
    bool musicLoaded = false;
    
    int enemiesDetectingPlayer = 0;

public:
    SFX() : criticalMusicPlaying(false), enemiesDetectingPlayer(0), musicLoaded(false) 
    {
        // Empty constructor
    }
    
    void initializeAudio()
    {
        // Load both background and critical theme music at the start
        if (!backgroundMusic.openFromFile("Assets/SoundTracks/PitcherPerfectTheme.wav")) 
        {
            std::cerr << "Failed to load background music!" << std::endl;
            return;
        }
        
        if (!criticalThemeMusic.openFromFile("Assets/SoundTracks/CriticalTheme.wav")) 
        {
            std::cerr << "Failed to load critical theme music!" << std::endl;
            return;
        }
        
        // Set up both music tracks
        backgroundMusic.setLooping(true);
        backgroundMusic.setVolume(100.f);
        
        criticalThemeMusic.setLooping(true);
        criticalThemeMusic.setVolume(100.f);
        
        // Start playing background music
        backgroundMusic.play();
        musicLoaded = true;
        std::cout << "Audio system initialized successfully" << std::endl;
    }
    
    void playCriticalTheme() 
    {
        if (!musicLoaded) return;
        
        if (!criticalMusicPlaying) 
        {
            std::cout << "Switching to critical theme" << std::endl;
            backgroundMusic.pause();
            criticalThemeMusic.play();
            criticalMusicPlaying = true;
        }
    }
    
    void playBackgroundMusic() {
        if (!musicLoaded) return;
        
        if (criticalMusicPlaying) {
            std::cout << "Switching to background music" << std::endl;
            criticalThemeMusic.stop();
            backgroundMusic.play();
            criticalMusicPlaying = false;
        }
    }
    
    void enemyDetectedPlayer() {
        if (!musicLoaded) return;
        
        enemiesDetectingPlayer++;
        std::cout << "Enemy detected player. Total detecting: " << enemiesDetectingPlayer << std::endl;
        
        // Always switch to critical theme when an enemy detects the player
        playCriticalTheme();
    }
    
    void enemyLostPlayer() {
        if (!musicLoaded) return;
        
        enemiesDetectingPlayer--;
        if (enemiesDetectingPlayer < 0) enemiesDetectingPlayer = 0;
        
        std::cout << "Enemy lost sight of player. Total detecting: " << enemiesDetectingPlayer << std::endl;
        
        // Only switch back to background music if no enemies are detecting
        if (enemiesDetectingPlayer == 0) {
            playBackgroundMusic();
        }
    }
    
    bool isCriticalMusicPlaying() const { return criticalMusicPlaying; }
    int getEnemiesDetectingCount() const { return enemiesDetectingPlayer; }
    bool isMusicLoaded() const { return musicLoaded; }
};

sf::Texture playerTexture;
sf::Sprite playerSprite(playerTexture);

// Global SFX instance
SFX sfx;

// Remove global sword variables and add Weapon class
class Weapon {
private:
    sf::Texture texture;
    std::optional<sf::Sprite> sprite;
    float scale;
    sf::Vector2f offset;
    float swingAngle = 0.f;
    bool swinging = false;
    sf::Clock swingClock;
    float swingDuration = 0.25f; // seconds for the swing 

public:
    Weapon(const std::string& texturePath, float scale = 1.0f) : scale(scale), offset(0.f, 0.f) {
        // First load the texture
        if (!texture.loadFromFile(texturePath)) {

            std::cerr << "Failed to load weapon texture from: " << texturePath << std::endl;
        }
        else {
            std::cout << "Weapon texture loaded successfully!" << std::endl;
            std::cout << "Weapon texture size: " << texture.getSize().x << "x" << texture.getSize().y << std::endl;

            // Create and set up the sprite after texture is loaded
            sprite.emplace(texture);
            sprite->setScale(sf::Vector2f(scale, scale));
            sprite->setOrigin(sf::Vector2f(texture.getSize().x / 2.f, texture.getSize().y / 2.f));
            /*
            // Debug output
            std::cout << "Sprite scale: " << sprite->getScale().x << "x" << sprite->getScale().y << std::endl;
            std::cout << "Sprite origin: " << sprite->getOrigin().x << "," << sprite->getOrigin().y << std::endl;
            */
        }
    }

    void updatePosition(const sf::Vector2f& playerPos, const sf::Vector2f& playerSize) {
        if (!sprite.has_value()) return;

        // Add offset to the right of the player
        sf::Vector2f newPos(
            playerPos.x + playerSize.x / 2 + 15.f,  // Add 15 pixels to the right
            playerPos.y + playerSize.y / 2
        );
        sprite->setPosition(newPos);

        /*
        // Debug output
        std::cout << "Weapon position: " << newPos.x << "," << newPos.y << std::endl;
        */
    }

    void draw(sf::RenderWindow& window) {
        if (sprite.has_value()) {
            window.draw(*sprite);
        }
    }

    sf::Sprite& getSprite() {
        if (!sprite.has_value()) {
            throw std::runtime_error("Sprite not initialized");
        }
        return *sprite;
    }

    void updateSwing(bool isAttacking) {
        if (isAttacking && !swinging) {
            swinging = true;
            swingClock.restart();
        }
        if (swinging) {
            float t = swingClock.getElapsedTime().asSeconds() / swingDuration;
            if (t < 1.f) {
                // Swing from -45 to +45 degrees and back to 0
                swingAngle = std::sin(t * 3.14159f) * 45.f;
            } else {
                swingAngle = 0.f;
                swinging = false;
            }
        } else {
            swingAngle = 0.f;
        }
        if (sprite.has_value()) {
            sprite->setRotation(sf::degrees(swingAngle));
        }
    }
  /*
    void updateMouseDirection(const sf::Vector2f& mousePos, const sf::Vector2f& playerPos, const sf::Vector2f& playerSize) {
        if (!sprite.has_value()) return;
        // Use the center of the player sprite for direction calculation
        sf::Vector2f playerCenter = playerPos + sf::Vector2f(playerSize.x / 2, playerSize.y / 2);
        sf::Vector2f direction = mousePos - playerCenter;
        float angle = std::atan2(direction.y, direction.x) * 180.f / 3.14159f;
        sprite->setRotation(sf::degrees(angle));
    }
    */
};

// Global MapManager instance
MapManager mapManager("Assets/maps");

// A* Pathfinding Implementation
struct Node {
    int x, y;
    float g_cost = 0;  // Cost from start to this node
    float h_cost = 0;  // Heuristic cost (estimated cost to goal)
    Node* parent = nullptr;

    float f_cost() const { return g_cost + h_cost; }

    bool operator==(const Node& other) const {
        return x == other.x && y == other.y;
    }
};

struct NodeHash {
    std::size_t operator()(const Node& node) const {
        return std::hash<int>()(node.x) ^ (std::hash<int>()(node.y) << 1);
    }
};

class PathFinder {
public:
    static std::vector<sf::Vector2f> findPath(const MapManager& mapManager,
        sf::Vector2f start,
        sf::Vector2f goal) {
        // Convert positions to tile coordinates
        int startX = static_cast<int>(start.x / SCALED_TILE_SIZE);
        int startY = static_cast<int>(start.y / SCALED_TILE_SIZE);
        int goalX = static_cast<int>(goal.x / SCALED_TILE_SIZE);
        int goalY = static_cast<int>(goal.y / SCALED_TILE_SIZE);

        // Create start and goal nodes
        Node startNode{ startX, startY };
        Node goalNode{ goalX, goalY };

        // Initialize open and closed sets
        std::vector<Node> openSet;
        std::unordered_map<Node, bool, NodeHash> closedSet;

        startNode.h_cost = calculateHeuristic(startNode, goalNode);
        openSet.push_back(startNode);

        while (!openSet.empty()) {
            // Find node with lowest f_cost
            auto current = std::min_element(openSet.begin(), openSet.end(),
                [](const Node& a, const Node& b) { return a.f_cost() < b.f_cost(); });

            Node currentNode = *current;

            if (currentNode.x == goalNode.x && currentNode.y == goalNode.y) {
                return reconstructPath(currentNode);
            }

            openSet.erase(current);
            closedSet[currentNode] = true;

            // Check all neighbors
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;

                    Node neighbor{ currentNode.x + dx, currentNode.y + dy };

                    // Skip if not passable or already in closed set
                    if (!mapManager.isTilePassable(neighbor.x, neighbor.y) ||
                        closedSet.find(neighbor) != closedSet.end()) {
                        continue;
                    }

                    float newGCost = currentNode.g_cost +
                        (dx != 0 && dy != 0 ? 1.414f : 1.0f); // Diagonal movement costs more

                    auto existingNode = std::find_if(openSet.begin(), openSet.end(),
                        [&neighbor](const Node& n) { return n.x == neighbor.x && n.y == neighbor.y; });

                    if (existingNode == openSet.end() || newGCost < existingNode->g_cost) {
                        neighbor.g_cost = newGCost;
                        neighbor.h_cost = calculateHeuristic(neighbor, goalNode);
                        neighbor.parent = new Node(currentNode);

                        if (existingNode == openSet.end()) {
                            openSet.push_back(neighbor);
                        }
                        else {
                            *existingNode = neighbor;
                        }
                    }
                }
            }
        }

        // No path found
        return std::vector<sf::Vector2f>();
    }

private:
    static float calculateHeuristic(const Node& a, const Node& b) {
        // Manhattan distance
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    static std::vector<sf::Vector2f> reconstructPath(const Node& endNode) {
        std::vector<sf::Vector2f> path;
        const Node* current = &endNode;

        while (current != nullptr) {
            path.push_back(sf::Vector2f(
                current->x * SCALED_TILE_SIZE + SCALED_TILE_SIZE / 2,
                current->y * SCALED_TILE_SIZE + SCALED_TILE_SIZE / 2
            ));
            current = current->parent;
        }

        std::reverse(path.begin(), path.end());
        return path;
    }
};

class Player
{
public:
    // Dash-related variables 
    bool isDashing = false;
    sf::Clock dashClock;
    sf::Clock dashDurationClock;

    sf::Clock damageCooldownClock;
    const float damageCooldown = 2.15f; // Half a second between hits

    // Attack-related variables
    sf::Clock attackCooldownClock;
    const float attackCooldown = 0.5f; // Half second between attacks
    bool isAttacking = false;
    const int attackDamage = 1;

    const float normalSpeed = 1.25f;
    const float dashSpeed = 5.75f;
    const float dashCooldown = 1.0f; // 1 second cooldown for dashing
    const float dashDuration = 0.15f; // 150 ms dash time

    sf::Vector2i size = sf::Vector2i(48, 64); // Size of the player sprite

    sf::Clock healingClock; // Tracks time for healing
    const float healingCooldown = 5.0f; // 15 seconds cooldown for healing

    // Animation textures
    sf::Texture playerTexture;
    sf::Texture idleTexture;

    sf::RectangleShape hitBox;
    int health = 10; // Player starts with 5 health points
    int score = 0;   // Player's score

    // Add death-related variables
    sf::Texture deathTexture;
    bool isDead = false;
    bool deathAnimationComplete = false;
    sf::Clock deathClock;
    const float deathDuration = 1.0f; // Duration of death animation in seconds
    int deathFrame = 0; // Frame counter for death animation

    Player()
    {
        hitBox.setFillColor(sf::Color::Transparent);
        hitBox.setOutlineColor(sf::Color::Red);
        hitBox.setOutlineThickness(1.0f);
    }
    void renderPlayer()
    {
        if (playerTexture.loadFromFile("Assets/Player/Textures/spritesheet2.png"))
        {
            std::cout << "The player has been loaded" << std::endl;

            playerSprite.setTexture(playerTexture);

            int xIndex = 0;
            int yIndex = 0;
            playerSprite.setTextureRect(sf::IntRect({ xIndex * size.x, yIndex * size.y }, { size.x, size.y }));
            playerSprite.scale({ 2.0f, 2.0f });
            playerSprite.setPosition({ 100, 100 });

            // Load idle texture
            if (idleTexture.loadFromFile("Assets/Player/Textures/Idle/idle_down.png"))
            {
                std::cout << "Idle texture loaded successfully" << std::endl;
            }
            else
            {
                std::cout << "Failed to load idle texture!" << std::endl;
            }

            // Load death texture
            if (deathTexture.loadFromFile("Assets/Player/Textures/Death/death_normal_down.png"))
            {
                std::cout << "Death texture loaded successfully" << std::endl;
            }
            else
            {
                std::cout << "Failed to load death texture!" << std::endl;
            }
        }
        else
        {
            std::cout << "Player not loaded!!" << std::endl;
        }
    }
    sf::Clock animationClock;
    const float animationDelay = 0.1f; // seconds
    int currentFrame = 0;
    void playerHealth(sf::RenderWindow& window)
    {
        sf::Texture healthTexture;
        if (!healthTexture.loadFromFile("Assets/UI/HeartIcons_32x32.png"))
        {
            std::cout << "Failed to load heart icon texture!" << std::endl;
            return;
        }

        // Draw hearts based on the player's current health
        for (int i = 0; i < health; ++i)
        {
            sf::Sprite healthSprite(healthTexture);
            healthSprite.setTextureRect(sf::IntRect({ 0, 0 }, { 32, 32 })); // Assuming the heart icon is at (0,0) in the texture
            healthSprite.setScale({ 0.75f, 0.75f });
            healthSprite.setPosition({ 10.f + i * 30.f, 10.f });
            window.draw(healthSprite);
        }
    }
    void playerMovement()
    {
        if (isDead) {
            // Handle death animation
            if (!deathAnimationComplete) {
                updateDeathAnimation();
                if (deathClock.getElapsedTime().asSeconds() > deathDuration) {
                    deathAnimationComplete = true;
                }
            }
            return; // Don't process movement if dead
        }

        static int frame = 0;
        static sf::Clock animationClock;
        static sf::Clock dashClock;
        static sf::Clock dashDurationClock;
        static bool isDashing = false;

        const float normalSpeed = 2.0f;
        const float dashSpeed = 6.5f;
        const float dashCooldown = 1.0f;
        const float dashDuration = 0.2f;

        float speed = normalSpeed;
        sf::Vector2f move(0.f, 0.f);
        int yIndex = 0;
        bool isMoving = false;

        // Attack input check using left mouse button
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && attackCooldownClock.getElapsedTime().asSeconds() > attackCooldown) {
            isAttacking = true;
            attackCooldownClock.restart();
            // Reset frame when starting a new attack
            frame = 0;
        }

        // Movement direction
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            move.x += 1;
            yIndex = isDashing ? 11 : (isAttacking ? 8 : 5); // Right dash/attack/walk
            isMoving = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            move.x -= 1;
            yIndex = isDashing ? 7 : (isAttacking ? 4 : 1); // Left dash/attack/walk
            isMoving = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        {
            move.y += 1;
            yIndex = isDashing ? 6 : (isAttacking ? 2 : 0); // Down dash/attack/walk
            isMoving = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        {
            move.y -= 1;
            yIndex = isDashing ? 9 : (isAttacking ? 10 : 3); // Up dash/attack/walk
            isMoving = true;
        }

        // Reset attack state after a short duration
        if (isAttacking && attackCooldownClock.getElapsedTime().asSeconds() > 0.2f) {
            isAttacking = false;
        }

        // Normalize diagonal movement
        if (move.x != 0 && move.y != 0)
            move /= std::sqrt(2.f);

        // Dash logic
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) && !isDashing && dashClock.getElapsedTime().asSeconds() > dashCooldown) {
            isDashing = true;
            dashDurationClock.restart();
            dashClock.restart();
        }

        if (isDashing)
        {
            speed = dashSpeed;
            if (dashDurationClock.getElapsedTime().asSeconds() > dashDuration)
            {
                isDashing = false;
            }
        }

        // Handle animation based on state
        if (isAttacking)
        {
            // Attack animation has fewer frames (typically 4-6)
            float frameDelay = 0.1f;
            int totalFrames = 6; // Adjust based on your attack animation frames

            if (animationClock.getElapsedTime().asSeconds() > frameDelay)
            {
                frame = (frame + 1) % totalFrames;
                animationClock.restart();
            }


            // Set sprite from the right row (yIndex) and frame
            playerSprite.setTextureRect(sf::IntRect({ frame * 48, yIndex * 64 }, { 48, 64 }));
        }
        else if (isMoving)
        {
            // Calculate new position
            sf::Vector2f newPosition = playerSprite.getPosition() + move * speed;

            // Check if the new position would be on a passable tile
            bool canMove = true;

            // Check horizontal movement
            if (move.x != 0) {
                float testX = newPosition.x;
                float testY = playerSprite.getPosition().y;

                // Check the center of the player sprite
                if (!mapManager.isPositionPassable(testX + playerSprite.getGlobalBounds().size.x / 2,
                    testY + playerSprite.getGlobalBounds().size.y / 2)) {
                    canMove = false;
                }
            }

            // Check vertical movement
            if (move.y != 0) {
                float testX = playerSprite.getPosition().x;
                float testY = newPosition.y;

                // Check the center of the player sprite
                if (!mapManager.isPositionPassable(testX + playerSprite.getGlobalBounds().size.x / 2,
                    testY + playerSprite.getGlobalBounds().size.y / 2)) {
                    canMove = false;
                }
            }

            // Only move if the tile is passable
            if (canMove) {
                playerSprite.move(move * speed);
            }

            float frameDelay = isDashing ? 0.08f : 0.1f;
            int totalFrames = 8; // Walking animation has 8 frames

            if (animationClock.getElapsedTime().asSeconds() > frameDelay)
            {
                frame = (frame + 1) % totalFrames;
                animationClock.restart();
            }

            // Use the main player texture for walking animation
            playerSprite.setTexture(playerTexture);
            // Set sprite from the right row (yIndex) and frame
            playerSprite.setTextureRect(sf::IntRect({ frame * 48, yIndex * 64 }, { 48, 64 }));
        }
        else
        {
            // Idle animation 
            float frameDelay = 0.2f;
            int totalFrames = 4; // Idle animation has 4 frames

            if (animationClock.getElapsedTime().asSeconds() > frameDelay)
            {
                frame = (frame + 1) % totalFrames;
                animationClock.restart();
            }

            // Use the idle texture for idle animation
            playerSprite.setTexture(idleTexture);
            playerSprite.setTextureRect(sf::IntRect({ frame * 48, 0 }, { 48, 64 }));
        }
    }

    void takeDamage(int damage)
    {
        if (isDead) return; // Don't take damage if already dead
        
        health -= damage;
        if (health < 0)
            health = 0; // Ensure health doesn't go below 0
        std::cout << "Player Health: " << health << std::endl;

        if (health <= 0) {
            isDead = true;
            deathClock.restart();
            deathFrame = 0; // Reset frame for death animation
            std::cout << "Player has died!" << std::endl;
        }

        healingClock.restart(); // Reset healing clock on damage
    }

    // friend function
    friend void updateHitBox(Player& player, Enemy& enemy);
    friend void updateHealing(Player& player);

    void updateDeathAnimation() {
        if (!deathAnimationComplete) {
            playerSprite.setTexture(deathTexture);
            // Assuming death animation has 6 frames
            int totalFrames = 6;
            float frameDelay = 0.1f;

            if (animationClock.getElapsedTime().asSeconds() > frameDelay) {
                if (deathFrame < totalFrames - 1) {
                    deathFrame++;
                }
                animationClock.restart();
            }

            playerSprite.setTextureRect(sf::IntRect({ deathFrame * size.x, 0 }, { size.x, size.y }));
        }
    }
}player;

// Declaration only
bool intersects(const sf::FloatRect& a, const sf::FloatRect& b, const Enemy& enemy, const Player& player);


class Enemy
{
    std::optional<sf::Sprite> enemySprite;
    sf::Texture idleTexture, walkTexture, attackTexture, hurtTexture, deathTexture;
    bool isGoblin = false;  // Flag to identify if this enemy is a goblin
    int attackDamage = 1;   // Default damage

    enum class State { Idle, Walk, Attack, Hurt, Dead };
    State currentState = State::Idle;

    // Add stun-related variables
    bool isStunned = false;
    sf::Clock stunClock;
    const float stunDuration = 1.0f; // .5 second stun duration

    int frame = 0;
    sf::Clock animationClock;
    float frameDelay = 0.1f;

    //  movement variables
    float speed = 1.25f;
    float detectionRange = 300.f;
    float attackRange = 50.f;
    const float pathUpdateInterval = 1.0f;
    const float smoothingFactor = 0.15f;
    sf::Vector2f currentVelocity = sf::Vector2f(0.f, 0.f);

    sf::Clock damageCooldownClock;
    const float damageCooldown = 0.5f; // Time between taking damage

    // Add a clock for hurt animation duration
    sf::Clock hurtClock;
    const float hurtDuration = 0.3f; // Duration of hurt animation in seconds

    // Add a clock for death animation duration
    sf::Clock deathClock;
    const float deathDuration = 1.0f; // Duration of death animation in seconds
    bool deathAnimationComplete = false;

    // New variables for pathfinding
    std::vector<sf::Vector2f> currentPath;
    size_t currentPathIndex = 0;
    sf::Clock pathUpdateClock;

    // Debug visualization
    std::vector<sf::CircleShape> pathVisualizers;
    bool showDebugPath = true;  // Set to true to see the path

    // Add stuck detection variables
    sf::Clock stuckTimer;
    sf::Vector2f lastPosition;
    const float stuckThreshold = 0.5f; // Time in seconds to consider enemy stuck
    bool isStuck = false;

    // Add path stability variables
    sf::Vector2f lastTargetPos;
    int consecutivePathChanges = 0;
    const int maxPathChanges = 3;
    sf::Clock pathChangeTimer;
    const float pathStabilityDelay = 1.0f;  // Minimum time between major path changes
    const float minPathLength = 32.0f;  // Minimum distance to consider a new path

    // Track if this enemy is currently detecting the player
    bool detectingPlayer = false;

public:
    int health = 5; // Enemy starts with 5 health
    bool isAlive = true;
    sf::RectangleShape hitBox;

    Enemy(bool isGoblinType = false) : isGoblin(isGoblinType)
    {
        hitBox.setFillColor(sf::Color::Transparent);
        hitBox.setOutlineColor(sf::Color::Red);
        hitBox.setOutlineThickness(1.0f);
        
        // Set higher damage for goblins
        if (isGoblin) {
            attackDamage = 2;  // Goblin deals 2 damage per hit
        }
    }

    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;
    Enemy(Enemy&&) = default;
    Enemy& operator=(Enemy&&) = default;

    void setPosition(float x, float y) {
        if (enemySprite) {
            enemySprite->setPosition(sf::Vector2f(x, y));
        }
    }

    void renderEnemy()
    {
        if (isGoblin) {
            if (!idleTexture.loadFromFile("Assets/Enemy/Golbin/Textures/spr_goblin_idle.png") ||
                !walkTexture.loadFromFile("Assets/Enemy/Golbin/Textures/spr_goblin_walk.png") ||
                !attackTexture.loadFromFile("Assets/Enemy/Golbin/Textures/spr_goblin_attack.png") ||
                !hurtTexture.loadFromFile("Assets/Enemy/Golbin/Textures/spr_goblin_hurt.png") ||
                !deathTexture.loadFromFile("Assets/Enemy/Golbin/Textures/spr_goblin_death.png")) {
                std::cout << "Failed to load goblin textures!" << std::endl;
                return;
            }

            enemySprite.emplace(idleTexture);
            enemySprite->setTextureRect(sf::IntRect({ 0, 0 }, { 64, 64 }));
            enemySprite->setPosition({ 400, 100 });
            enemySprite->scale({ 2.0f, 2.0f });

            // Adjust the hitBox size for goblin
            hitBox.setSize({ 64.f * enemySprite->getScale().x * 0.3f,
                            64.f * enemySprite->getScale().y * 0.3f });
        } else {
            // Original slime enemy code
            if (!idleTexture.loadFromFile("Assets/Enemy/Slime/Blue Slime/Textures/shadowless/spr_Blue_slime_idle_shadowless.png") ||
                !walkTexture.loadFromFile("Assets/Enemy/Slime/Blue Slime/Textures/shadowless/spr_Blue_slime_walk_shadowless.png") ||
                !attackTexture.loadFromFile("Assets/Enemy/Slime/Blue Slime/Textures/shadowless/spr_Blue_slime_attack_shadowless.png") ||
                !hurtTexture.loadFromFile("Assets/Enemy/Slime/Blue Slime/Textures/spr_Blue_slime_hurt.png") ||
                !deathTexture.loadFromFile("Assets/Enemy/Slime/Blue Slime/Textures/spr_Blue_slime_death.png"))
            {
                std::cout << "Failed to load one or more slime textures!" << std::endl;
                return;
            }

            enemySprite.emplace(idleTexture);
            enemySprite->setTextureRect(sf::IntRect({ 0, 0 }, { 64, 64 }));
            enemySprite->setPosition({ 400, 100 });
            enemySprite->scale({ 2.0f, 2.0f });

            // Adjust the hitBox size for slime
            hitBox.setSize({ 64.f * enemySprite->getScale().x * 0.3f,
                            64.f * enemySprite->getScale().y * 0.3f });
        }
    }

    void draw(sf::RenderWindow& window)
    {
        if (!enemySprite) return;

        // Don't draw if death animation is complete
        if (currentState == State::Dead && deathAnimationComplete) return;

        // Draw the sprite
        window.draw(*enemySprite);
       
        // draw debug path if alive
        /*
          if (showDebugPath && isAlive)
         {
            for (const auto& point : currentPath) 
            {
                sf::CircleShape circle(5.0f);
                circle.setFillColor(sf::Color(255, 0, 0, 128));
                circle.setPosition(sf::Vector2f(point.x - 5.0f, point.y - 5.0f));
                window.draw(circle);
            }
          }

		  */
    }

    // Add line of sight check method
    bool hasLineOfSight(sf::Vector2f start, sf::Vector2f end) {
        // Get the direction vector
        sf::Vector2f direction = end - start;
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // Normalize direction
        direction.x /= distance;
        direction.y /= distance;

        // Check points along the line
        const float checkInterval = 16.0f; // Check every 16 pixels
        float currentDist = 0.0f;

        while (currentDist < distance) {
            sf::Vector2f checkPoint = start + direction * currentDist;
            if (!mapManager.isPositionPassable(checkPoint.x, checkPoint.y)) {
                return false;
            }
            currentDist += checkInterval;
        }

        return true;
    }

    void enemyMovement(sf::Vector2f playerPos) {
        if (!enemySprite) return;

        // Handle death state
        if (currentState == State::Dead) {
            if (!deathAnimationComplete) {
                updateAnimation();
                if (deathClock.getElapsedTime().asSeconds() > deathDuration) {
                    deathAnimationComplete = true;
                    enemySprite.reset();
                    // Only switch back to background music when the enemy is completely dead
                    if (sfx.isCriticalMusicPlaying()) {
                        sfx.playBackgroundMusic();
                    }
                }
            }
            return;
        }

        // Handle hurt state
        if (currentState == State::Hurt) {
            updateAnimation();
            // Return to previous state after hurt animation
            if (hurtClock.getElapsedTime().asSeconds() > hurtDuration) {
                currentState = State::Idle;
                isStunned = false;  // End stun when hurt animation ends
            }
            return;  // Don't process other states while in hurt state
        }

        // Handle stun state
        if (isStunned) {
            if (stunClock.getElapsedTime().asSeconds() >= stunDuration) {
                isStunned = false;
            }
            else {
                // While stunned, only update animation but don't move or attack
                updateAnimation();
                return;
            }
        }

        // Only process movement if alive and not in hurt state
        if (!isAlive) return;

        // Get the center positions of both sprites
        sf::Vector2f slimePos = enemySprite->getPosition() + sf::Vector2f(
            enemySprite->getGlobalBounds().size.x / 2,
            enemySprite->getGlobalBounds().size.y / 2
        );

        sf::Vector2f playerCenterPos = playerPos + sf::Vector2f(
            playerSprite.getGlobalBounds().size.x / 2,
            playerSprite.getGlobalBounds().size.y / 2
        );

        float distance = std::sqrt(
            std::pow(playerCenterPos.x - slimePos.x, 2) +
            std::pow(playerCenterPos.y - slimePos.y, 2)
        );

        // Check line of sight to player using center positions
        bool canSeePlayer = false;
        if (distance < detectionRange) {
            canSeePlayer = hasLineOfSight(slimePos, playerCenterPos);
        }

        // Update detection state using SFX class
        if (!detectingPlayer && canSeePlayer) {
            sfx.enemyDetectedPlayer();
        } else if (detectingPlayer && !canSeePlayer) {
            // Only switch back to background music if no enemies are detecting
            if (sfx.getEnemiesDetectingCount() <= 1) {  // This enemy is about to stop detecting
                sfx.enemyLostPlayer();
            }
        }
        detectingPlayer = canSeePlayer; // Update for next frame

        // State management
        if (currentState != State::Hurt && currentState != State::Dead) {
            if (canSeePlayer) {
                if (distance < attackRange) {
                    currentState = State::Attack;
                    currentPath.clear();
                    currentVelocity = sf::Vector2f(0.f, 0.f);
                }
                else if (distance < detectionRange) {
                    currentState = State::Walk;

                    // Always try to find a path if we don't have one
                    if (currentPath.empty()) {
                        currentPath = PathFinder::findPath(mapManager, slimePos, playerCenterPos);
                        currentPathIndex = 0;
                    }
                    // Update path periodically or if we're stuck
                    else if (pathUpdateClock.getElapsedTime().asSeconds() >= pathUpdateInterval) {
                        // Check if we're making progress
                        float progressDistance = std::sqrt(
                            std::pow(currentPath[currentPathIndex].x - slimePos.x, 2) +
                            std::pow(currentPath[currentPathIndex].y - slimePos.y, 2)
                        );

                        // If we're not getting closer to the next point, try a new path
                        if (progressDistance > 32.0f) {
                            currentPath = PathFinder::findPath(mapManager, slimePos, playerCenterPos);
                            currentPathIndex = 0;
                        }
                        pathUpdateClock.restart();
                    }
                }
            }
            else {
                // Lost sight of player, go idle
                currentState = State::Idle;
                currentPath.clear();
                currentVelocity = sf::Vector2f(0.f, 0.f);
            }
        }

        // Movement logic - only move if we can see the player
        if (canSeePlayer && currentState == State::Walk && !currentPath.empty() && currentPathIndex < currentPath.size()) {
            // Get current target point
            sf::Vector2f targetPos = currentPath[currentPathIndex];

            // Calculate distance to current target
            float distanceToTarget = std::sqrt(
                std::pow(targetPos.x - slimePos.x, 2) +
                std::pow(targetPos.y - slimePos.y, 2)
            );

            // Move to next path point if close enough
            if (distanceToTarget < 8.0f) {
                currentPathIndex++;
                if (currentPathIndex >= currentPath.size()) {
                    // If we've reached the end of the path but still can see the player,
                    // try to find a new path to the player's current position
                    if (canSeePlayer && distance < detectionRange) {
                        currentPath = PathFinder::findPath(mapManager, slimePos, playerCenterPos);
                        currentPathIndex = 0;
                    }
                    else {
                        currentPath.clear();
                    }
                    return;
                }
                targetPos = currentPath[currentPathIndex];
            }

            // Calculate movement direction
            sf::Vector2f direction = targetPos - slimePos;
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (length > 0) {
                direction.x /= length;
                direction.y /= length;
            }

            // Calculate target velocity
            sf::Vector2f targetVelocity = direction * speed;

            // Smooth the velocity changes
            currentVelocity.x = currentVelocity.x * (1 - smoothingFactor) + targetVelocity.x * smoothingFactor;
            currentVelocity.y = currentVelocity.y * (1 - smoothingFactor) + targetVelocity.y * smoothingFactor;

            // Calculate new position
            sf::Vector2f newPos = enemySprite->getPosition() + currentVelocity;

            // Enhanced collision detection with multiple checks
            bool canMove = true;
            const float checkRadius = 16.0f;  // Reduced from 24.0f to allow for tighter movement

            // Check if the new position would be valid
            sf::Vector2f testPos = newPos;

            // Check center point
            if (!mapManager.isPositionPassable(testPos.x + enemySprite->getGlobalBounds().size.x / 2,
                testPos.y + enemySprite->getGlobalBounds().size.y / 2)) {
                canMove = false;
            }

            // If we can't move directly, try to slide along walls
            if (!canMove) {
                // Try horizontal movement
                sf::Vector2f horizontalMove = sf::Vector2f(currentVelocity.x, 0);
                sf::Vector2f horizontalPos = enemySprite->getPosition() + horizontalMove;

                if (mapManager.isPositionPassable(horizontalPos.x + enemySprite->getGlobalBounds().size.x / 2,
                    horizontalPos.y + enemySprite->getGlobalBounds().size.y / 2)) {
                    newPos = horizontalPos;
                    canMove = true;
                }
                else {
                    // Try vertical movement
                    sf::Vector2f verticalMove = sf::Vector2f(0, currentVelocity.y);
                    sf::Vector2f verticalPos = enemySprite->getPosition() + verticalMove;

                    if (mapManager.isPositionPassable(verticalPos.x + enemySprite->getGlobalBounds().size.x / 2,
                        verticalPos.y + enemySprite->getGlobalBounds().size.y / 2)) {
                        newPos = verticalPos;
                        canMove = true;
                    }
                }
            }

            // If still stuck, force a path recalculation
            if (!canMove) {
                currentPath.clear();
                currentVelocity = sf::Vector2f(0.f, 0.f);

                // Try to find an alternative path
                sf::Vector2f offset(32.f, 0.f);  // Try offset positions
                std::vector<sf::Vector2f> alternativeTargets = {
                    playerCenterPos + offset,
                    playerCenterPos - offset,
                    playerCenterPos + sf::Vector2f(0.f, 32.f),
                    playerCenterPos + sf::Vector2f(0.f, -32.f)
                };

                for (const auto& target : alternativeTargets) {
                    if (mapManager.isPositionPassable(target.x, target.y)) {
                        currentPath = PathFinder::findPath(mapManager, slimePos, target);
                        if (!currentPath.empty()) {
                            currentPathIndex = 0;
                            break;
                        }
                    }
                }
            }

            if (canMove) {
                enemySprite->setPosition(newPos);
                lastPosition = newPos;  // Update last position
                stuckTimer.restart();   // Reset stuck timer
            }
            else {
                // Check if we're stuck
                if (stuckTimer.getElapsedTime().asSeconds() > stuckThreshold) {
                    // If stuck for too long, try to find a path to a random nearby position
                    sf::Vector2f randomOffset(
                        (std::rand() % 64) - 32.f,
                        (std::rand() % 64) - 32.f
                    );
                    sf::Vector2f escapeTarget = slimePos + randomOffset;

                    if (mapManager.isPositionPassable(escapeTarget.x, escapeTarget.y)) {
                        currentPath = PathFinder::findPath(mapManager, slimePos, escapeTarget);
                        currentPathIndex = 0;
                    }

                    stuckTimer.restart();
                }
            }

            // Reduce speed when close to walls
            float wallProximityThreshold = 32.0f;
            bool nearWall = false;
            std::vector<sf::Vector2f> checkPoints = {
                {newPos.x - wallProximityThreshold, newPos.y},
                {newPos.x + wallProximityThreshold, newPos.y},
                {newPos.x, newPos.y - wallProximityThreshold},
                {newPos.x, newPos.y + wallProximityThreshold}
            };

            for (const auto& point : checkPoints) {
                if (!mapManager.isPositionPassable(point.x, point.y)) {
                    nearWall = true;
                    break;
                }
            }

            if (nearWall) {
                speed = 0.75f;  // Reduce speed near walls
            }
            else {
                speed = 1.25f;  // Normal speed in open areas
            }
        }

        // Animation update
        updateAnimation();
    }

    void takeDamage(int damage)
    {
        if (!isAlive || damageCooldownClock.getElapsedTime().asSeconds() < damageCooldown)
            return;

        health -= damage;

        if (health <= 0) {
            isAlive = false;
            health = 0;
            currentState = State::Dead;
            deathClock.restart();
            frame = 0;  // Reset frame for death animation
            currentPath.clear();
            currentVelocity = sf::Vector2f(0.f, 0.f);
            
            // Increase player score when enemy dies
            if (isGoblin) {  // If it's a goblin
                player.score += 20;  // Add 20 points for goblin kill
                std::cout << "Goblin defeated! Score increased by 20. Total score: " << player.score << std::endl;
            } else {  // If it's a slime
                player.score += 10;  // Add 10 points for slime kill
                std::cout << "Slime defeated! Score increased by 10. Total score: " << player.score << std::endl;
            }
            
            std::cout << "Enemy defeated!" << std::endl;
        }
        else {
            // Only apply stun and set hurt state if not already in hurt state
            if (currentState != State::Hurt) {
                isStunned = true;
                stunClock.restart();
                currentState = State::Hurt;
                hurtClock.restart();
                frame = 0;  // Reset frame for hurt animation
                std::cout << "Enemy Health: " << health << std::endl;
            }
        }
        damageCooldownClock.restart();
    }

    bool isDeathAnimationComplete() const {
        return deathAnimationComplete;
    }

    friend void updateHitBox(Player& player, Enemy& enemy);

private:
    void updateAnimation() {
        if (!enemySprite) return;

        int totalFrames = 0;
        float currentFrameDelay = frameDelay;  // Default frame delay

        if (isGoblin) {
            // Goblin animation logic
            switch (currentState) {
                case State::Dead:
                    enemySprite->setTexture(deathTexture);
                    totalFrames = 4;  // Goblin death animation has 4 frames
                    if (frame >= totalFrames - 1) {
                        frame = totalFrames - 1;  // Stay on last frame
                        return;
                    }
                    break;
                case State::Hurt:
                    enemySprite->setTexture(hurtTexture);
                    totalFrames = 4;  // Goblin hurt animation has 4 frames
                    break;
                case State::Attack:
                    enemySprite->setTexture(attackTexture);
                    totalFrames = 14;  // Goblin attack animation has 14 frames
                    currentFrameDelay = frameDelay * 0.5f;  // Make attack animation twice as fast
                    // Reset frame when starting attack
                    if (frame >= totalFrames) {
                        frame = 0;
                    }
                    break;
                case State::Walk:
                    enemySprite->setTexture(walkTexture);
                    totalFrames = 4;  // Goblin walk animation has 4 frames
                    break;
                default:
                    enemySprite->setTexture(idleTexture);
                    totalFrames = 4;  // Goblin idle animation has 4 frames
                    break;
            }

            if (animationClock.getElapsedTime().asSeconds() > currentFrameDelay) {
                if (currentState == State::Dead) {
                    if (frame < totalFrames - 1) {
                        frame++;
                    }
                } else if (currentState == State::Attack) {
                    // For attack animation, play once and return to idle
                    if (frame < totalFrames - 1) {
                        frame++;
                    } else {
                        currentState = State::Idle;
                        frame = 0;
                    }
                } else {
                    frame = (frame + 1) % totalFrames;
                }
                animationClock.restart();
            }

            enemySprite->setTextureRect(sf::IntRect({ frame * 64, 0 }, { 64, 64 }));
        } else {
            // Original slime animation logic
            switch (currentState) {
                case State::Dead:
                    enemySprite->setTexture(deathTexture);
                    totalFrames = 6;
                    if (frame >= totalFrames - 1) {
                        frame = totalFrames - 1;
                        return;
                    }
                    break;
                case State::Idle:
                    enemySprite->setTexture(idleTexture);
                    totalFrames = 6;
                    break;
                case State::Walk:
                    enemySprite->setTexture(walkTexture);
                    totalFrames = 6;
                    break;
                case State::Attack:
                    enemySprite->setTexture(attackTexture);
                    totalFrames = 15;
                    break;
                case State::Hurt:
                    enemySprite->setTexture(hurtTexture);
                    totalFrames = 4;
                    break;
            }

            if (animationClock.getElapsedTime().asSeconds() > frameDelay) {
                if (currentState == State::Dead) {
                    if (frame < totalFrames - 1) {
                        frame++;
                    }
                } else {
                    frame = (frame + 1) % totalFrames;
                }
                animationClock.restart();
            }

            enemySprite->setTextureRect(sf::IntRect({ frame * 64, 0 }, { 64, 64 }));
        }
    }
}enemy;

// List for enemies
std::list<Enemy> enemies;
const int MAX_ENEMIES = 5; // Maximum number of enemies to spawn

// Update spawnEnemy function to spawn specific enemy types based on level
void spawnEnemy() {
    if (enemies.size() >= MAX_ENEMIES) return;

    // Determine enemy type based on current level
    bool isGoblin = (mapManager.getCurrentMapNumber() == 2);
    Enemy newEnemy(isGoblin);
    newEnemy.renderEnemy();

    // Keep trying to find a valid spawn position
    float randomX, randomY;
    bool validPosition = false;
    int maxAttempts = 100;
    int attempts = 0;

    while (!validPosition && attempts < maxAttempts) {
        randomX = static_cast<float>(rand() % (MAP_WIDTH_PIXELS - 100));
        randomY = static_cast<float>(rand() % (MAP_HEIGHT_PIXELS - 100));

        if (mapManager.isPositionPassable(randomX + 32.f, randomY + 32.f)) {
            validPosition = true;
        }
        attempts++;
    }

    if (validPosition) {
        newEnemy.setPosition(randomX, randomY);
        enemies.emplace_back(std::move(newEnemy));
        std::cout << (isGoblin ? "Goblin" : "Slime") << " spawned at (" << randomX << ", " << randomY << ")! Total enemies: " << enemies.size() << std::endl;
    } else {
        std::cout << "Failed to find valid spawn position for enemy after " << maxAttempts << " attempts." << std::endl;
    }
}

// Define the intersects function after the Enemy class is fully defined
bool intersects(const sf::FloatRect& a, const sf::FloatRect& b, const Enemy& enemy, const Player& player)
{
    sf::FloatRect boundsE = enemy.hitBox.getGlobalBounds();
    sf::FloatRect boundsP = player.hitBox.getGlobalBounds();

    float leftP = boundsP.position.x;
    float topP = boundsP.position.y;
    float widthP = boundsP.size.x;
    float heightP = boundsP.size.y;

    float leftE = boundsE.position.x;
    float topE = boundsE.position.y;
    float widthE = boundsE.size.x;
    float heightE = boundsE.size.y;
    return (leftP < leftE + widthE &&
        leftP + widthP > leftE &&
        topP < topE + heightE &&
        topP + heightP > topE);
}

void updateHitBox(Player& player, Enemy& enemy)
{
    // Update Player HitBox - always update regardless of enemy state
    player.hitBox.setSize({ static_cast<float>(player.size.x) * playerSprite.getScale().x * 0.4f,
                        static_cast<float>(player.size.y) * playerSprite.getScale().y * 0.4f });

    // Calculate hitbox position based on player sprite position
    float offsetX = 30.f;
    float offsetY = 35.f;

    player.hitBox.setPosition(sf::Vector2f(playerSprite.getPosition().x + offsetX, playerSprite.getPosition().y + offsetY));

    // Only process enemy-related collision if enemy is alive
    if (!enemy.isAlive) return;

    // Update Enemy HitBox
    if (enemy.enemySprite)
    {
        enemy.hitBox.setSize({ 64.f * enemy.enemySprite->getScale().x * 0.3f,
                            64.f * enemy.enemySprite->getScale().y * 0.3f });
        enemy.hitBox.setPosition(sf::Vector2f(enemy.enemySprite->getPosition().x + 40.f, enemy.enemySprite->getPosition().y + 40.f));
    }

    // Check for collision and apply damage
    if (intersects(player.hitBox.getGlobalBounds(), enemy.hitBox.getGlobalBounds(), enemy, player))
    {
        // Enemy damages player
        if (enemy.currentState == Enemy::State::Attack &&
            player.damageCooldownClock.getElapsedTime().asSeconds() > player.damageCooldown)
        {
            player.takeDamage(enemy.attackDamage);  // Use enemy's attackDamage
            player.damageCooldownClock.restart();
        }

        // Player damages enemy
        if (player.isAttacking)
        {
            enemy.takeDamage(player.attackDamage);
        }
    }
}

void updateHealing(Player& player)
{
    if (player.health < 10 && player.healingClock.getElapsedTime().asSeconds() > player.healingCooldown)
    {
        player.health += 1; // Heal 1 health point
        player.healingClock.restart(); // Restart the healing clock
        std::cout << "Player healed! Current Health: " << player.health << std::endl;
    }
}

// Add function to check if all enemies are dead
bool areAllEnemiesDead() {
    for (const auto& enemy : enemies) {
        if (enemy.isAlive) {
            return false;
        }
    }
    return true;
}


std::string forestTilesheet = "Assets/Map/Fantasy/forest_/forest_1.png";
std::string tundraTilesheet = "Assets/Map/Fantasy/tundra_/tundra_.png";

// Declare mapTilesheet as a global variable
sf::Texture mapTilesheet;

// Update loadNextMap to also switch tilesheet
void loadNextMap() {
    int currentMap = mapManager.getCurrentMapNumber();
    int nextMap = currentMap + 1;
    
    // Save current player position
    sf::Vector2f currentPlayerPos = playerSprite.getPosition();
    
    // Set correct tilesheet for the map
    std::string newTilesheetPath = forestTilesheet;
    if (nextMap == 2) {
        newTilesheetPath = tundraTilesheet;
    }
    if (!mapTilesheet.loadFromFile(newTilesheetPath)) {
        std::cerr << "Failed to load map tilesheet: " << newTilesheetPath << std::endl;
    }
    
    // Try to load the next map
    if (mapManager.loadMap(nextMap)) {
        // Reset player position to a safe starting position
        playerSprite.setPosition({100, 100});
        
        // Clear existing enemies
        enemies.clear();
        
        // Spawn new enemies for the new map
        for (int i = 0; i < 5; ++i) {
            spawnEnemy();
        }
        
        std::cout << "Successfully loaded map " << nextMap << " after clearing all enemies!" << std::endl;
    } else {
        // If map loading failed, restore player position
        playerSprite.setPosition(currentPlayerPos);
        std::cerr << "Failed to load next map " << nextMap << std::endl;
    }
}

// Remove the drawMapInfo function and replace it with a simpler version
void drawMapInfo(sf::RenderWindow& window, const MapManager& mapManager) {
   
    static bool infoPrinted = false;

    if (!infoPrinted) {
        std::string filename = mapManager.getCurrentMapFilename();
        std::filesystem::path path(filename);
        std::string mapName = path.filename().string();

        std::cout << "Current map: " << mapManager.getCurrentMapNumber() << " (" << mapName << ")" << std::endl;
        std::cout << "Press 1-9 to switch maps" << std::endl;

        infoPrinted = true;
    }

    // Draw debug visualization for collision detection
    sf::CircleShape debugPoint(3.0f);
    debugPoint.setFillColor(sf::Color::Red);

    // Draw the center point of the player for collision detection
    float centerX = playerSprite.getPosition().x + playerSprite.getGlobalBounds().size.x / 2;
    float centerY = playerSprite.getPosition().y + playerSprite.getGlobalBounds().size.y / 2;
    debugPoint.setPosition(sf::Vector2f(centerX, centerY));

}

// Audio functions have been moved to the SFX class

// Add font and text variables after the global variables
sf::Font gameFont;
sf::Text scoreText(gameFont);
sf::Text mapText(gameFont);
sf::Text titleText(gameFont);
sf::Text pressStartText(gameFont);
bool gameStarted = false;
float titleAlpha = 0.f;
float pressStartAlpha = 0.f;
sf::Clock titleClock;

sf::Text levelClearedText(gameFont);
sf::Text pressContinueText(gameFont);
bool levelCleared = false;
float levelClearedAlpha = 0.f;
float pressContinueAlpha = 0.f;
sf::Clock levelClearedClock;


sf::Text gameOverText(gameFont);
sf::Text finalScoreText(gameFont);
sf::Text pressExitText(gameFont);
bool gameOver = false;
float gameOverAlpha = 0.f;
float pressExitAlpha = 0.f;
sf::Clock gameOverClock;


sf::Text toBeContinuedText(gameFont);
bool toBeContinued = false;
float toBeContinuedAlpha = 0.f;
sf::Clock toBeContinuedClock;

int main()  
{  
   // Initialize random seed  
   srand(static_cast<unsigned int>(time(nullptr)));  

   // Initialize  
   sf::ContextSettings settings;  
   settings.antiAliasingLevel = 8;  

   sf::RenderWindow window(sf::VideoMode({ 1280,720 }), "AshVale", sf::Style::Default, sf::State::Windowed, settings);
   window.setFramerateLimit(60);

   bool gameRunning = true;  

   // Load font
   if (!gameFont.openFromFile("Assets/Font/PIXBOB MINI LITE.ttf")) {
       std::cerr << "Failed to load font!" << std::endl;
       return 1;
   }

   // Set up title text
   titleText.setCharacterSize(96);  // Increased from 64 to 96
   titleText.setFillColor(sf::Color::Black);
   titleText.setString("ASHVALE");
   titleText.setPosition(sf::Vector2f(
       window.getSize().x / 2.f - titleText.getGlobalBounds().size.x / 2.f,
       window.getSize().y / 3.f
   ));

   // Set up press start text
   pressStartText.setCharacterSize(32);
   pressStartText.setFillColor(sf::Color::Black);
   pressStartText.setString("Press SPACE to Start");
   pressStartText.setPosition(sf::Vector2f(
       window.getSize().x / 2.f - pressStartText.getGlobalBounds().size.x / 2.f,
       window.getSize().y * 2.f / 3.f
   ));

   // Set up score text
   scoreText.setCharacterSize(32);
   scoreText.setFillColor(sf::Color::Black);
   scoreText.setPosition(sf::Vector2f(window.getSize().x - 200.f, 40.f));
   scoreText.setString("Score: 0");

   // Level text
   mapText.setCharacterSize(32);
   mapText.setFillColor(sf::Color::Black);
   mapText.setPosition(sf::Vector2f(window.getSize().x - 200.f, 70.f));
   mapText.setString("Level: 1");

   // Set up level cleared text
   levelClearedText.setCharacterSize(64);
   levelClearedText.setFillColor(sf::Color::Black);
   levelClearedText.setString("LEVEL CLEARED");
   levelClearedText.setPosition(sf::Vector2f(
       window.getSize().x / 2.f - levelClearedText.getGlobalBounds().size.x / 2.f,
       window.getSize().y / 3.f
   ));

   // Set up press continue text
   pressContinueText.setCharacterSize(32);
   pressContinueText.setFillColor(sf::Color::Black);
   pressContinueText.setString("Press SPACE to Continue");
   pressContinueText.setPosition(sf::Vector2f(
       window.getSize().x / 2.f - pressContinueText.getGlobalBounds().size.x / 2.f,
       window.getSize().y * 2.f / 3.f
   ));

   // Set up game over text
   gameOverText.setCharacterSize(96);
   gameOverText.setFillColor(sf::Color::Black);
   gameOverText.setString("GAME OVER");
   gameOverText.setPosition(sf::Vector2f(
       window.getSize().x / 2.f - gameOverText.getGlobalBounds().size.x / 2.f,
       window.getSize().y / 3.f
   ));

   // Set up final score text
   finalScoreText.setCharacterSize(48);
   finalScoreText.setFillColor(sf::Color::Black);
   finalScoreText.setPosition(sf::Vector2f(
       window.getSize().x / 2.f - finalScoreText.getGlobalBounds().size.x / 2.f,
       window.getSize().y / 2.f
   ));

   // Set up press exit text
   pressExitText.setCharacterSize(32);
   pressExitText.setFillColor(sf::Color::Black);
   pressExitText.setString("Press SPACE to Exit");
   pressExitText.setPosition(sf::Vector2f(
       window.getSize().x / 2.f - pressExitText.getGlobalBounds().size.x / 2.f,
       window.getSize().y * 2.f / 3.f
   ));

   // Set up to be continued text
   toBeContinuedText.setCharacterSize(96);
   toBeContinuedText.setFillColor(sf::Color::Black);
   toBeContinuedText.setString("TO BE CONTINUED");
   toBeContinuedText.setPosition(sf::Vector2f(
       window.getSize().x / 2.f - toBeContinuedText.getGlobalBounds().size.x / 2.f,
       window.getSize().y / 3.f
   ));

   // Load map tilesheet  
   std::string tilesheetPath = forestTilesheet;
   if (!mapTilesheet.loadFromFile(tilesheetPath)) {  
       std::cerr << "Failed to load map tilesheet: " << tilesheetPath << std::endl;  
       return 1;  
   }  

   // Create weapon instance  
   Weapon sword("Assets/32 Free Weapon Icons/Icons/Iicon_32_38.png");  

   // Load initial map  
   if (!mapManager.loadMap(1)) {
       std::cerr << "Failed to load initial map!" << std::endl;
       return 1;
   }

   // Load player  
   player.renderPlayer();  
   playerSprite.setPosition({100, 100}); // Set initial player position
   
   // Initialize audio
   sfx.initializeAudio();
   
   // Spawn initial enemies only once at the start
   for (int i = 0; i < 5; ++i) {
       spawnEnemy();
   }

   // Game Loop (infinite loop)  
   while (window.isOpen())  
   {  
       // Event handling  
       while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            // Handle space key to start game, continue after level cleared, or exit after game over/to be continued
            if ((!gameStarted || levelCleared || gameOver || toBeContinued) && event->is<sf::Event::KeyPressed>()) {
                const auto& keyEvent = *event->getIf<sf::Event::KeyPressed>();
                if (keyEvent.code == sf::Keyboard::Key::Space) {
                    if (!gameStarted) {
                        gameStarted = true;
                    } else if (levelCleared) {
                        levelCleared = false;
                        loadNextMap();
                    } else if (gameOver || toBeContinued) {
                        gameRunning = false;  // Set gameRunning to false to exit the game loop
                        window.close();      // Close the window
                        break;               // Break out of the event loop
                    }
                }
            }

            // Handle map switching with number keys 
            if (event->is<sf::Event::KeyPressed>()) {
                const auto& keyEvent = *event->getIf<sf::Event::KeyPressed>();
                if (keyEvent.code >= sf::Keyboard::Key::Num0 && keyEvent.code <= sf::Keyboard::Key::Num9) {
                    int mapNumber = static_cast<int>(keyEvent.code) - static_cast<int>(sf::Keyboard::Key::Num0);
                    if (mapNumber == 0) mapNumber = 10; // Handle 0 key as map 10
                    
                    // Save current player position
                    sf::Vector2f currentPlayerPos = playerSprite.getPosition();
                    
                    // Set correct tilesheet for the map
                    std::string newTilesheetPath = forestTilesheet;
                    if (mapNumber == 2) {
                        newTilesheetPath = tundraTilesheet;
                    }
                    if (!mapTilesheet.loadFromFile(newTilesheetPath)) {
                        std::cerr << "Failed to load map tilesheet: " << newTilesheetPath << std::endl;
                    }
                    
                    // Try to load the new map
                    if (mapManager.loadMap(mapNumber)) {
                        // Reset player position to a safe starting position
                        playerSprite.setPosition({100, 100});
                        
                        // Clear existing enemies
                        enemies.clear();
                        
                        // Spawn new enemies for the new map
                        for (int i = 0; i < 5; ++i) {
                            spawnEnemy();
                        }
                        
                        std::cout << "Successfully switched to map " << mapNumber << std::endl;
                    } else {
                        // If map loading failed, restore player position
                        playerSprite.setPosition(currentPlayerPos);
                        std::cerr << "Failed to load map " << mapNumber << std::endl;
                    }
                }
            }
        }

       // Clear the window  
       window.clear(sf::Color::White);  

       if (!gameRunning) {  // Check if game should stop
           break;           // Break out of the main game loop
       }

       if (!gameStarted) {
           // Title screen animation
           float elapsed = titleClock.getElapsedTime().asSeconds();
           
           // Fade in title
           if (titleAlpha < 255.f) {
               titleAlpha = std::min(255.f, elapsed * 100.f);
               titleText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(titleAlpha)));
           }
           
           // Blink "Press Start" text
           pressStartAlpha = 128.f + 127.f * std::sin(elapsed * 2.f);
           pressStartText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(pressStartAlpha)));

           // Draw title screen
           window.draw(titleText);
           window.draw(pressStartText);
       } else if (levelCleared) {
           // Level cleared screen animation
           float elapsed = levelClearedClock.getElapsedTime().asSeconds();
           
           // Fade in level cleared text
           if (levelClearedAlpha < 255.f) {
               levelClearedAlpha = std::min(255.f, elapsed * 100.f);
               levelClearedText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(levelClearedAlpha)));
           }
           
           // Blink "Press Continue" text
           pressContinueAlpha = 128.f + 127.f * std::sin(elapsed * 2.f);
           pressContinueText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(pressContinueAlpha)));

           // Draw level cleared screen
           window.draw(levelClearedText);
           window.draw(pressContinueText);
       } else if (gameOver) {
           // Game over screen animation
           float elapsed = gameOverClock.getElapsedTime().asSeconds();
           
           // Fade in game over text
           if (gameOverAlpha < 255.f) {
               gameOverAlpha = std::min(255.f, elapsed * 100.f);
               gameOverText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(gameOverAlpha)));
               finalScoreText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(gameOverAlpha)));
           }
           
           // Blink "Press Exit" text
           pressExitAlpha = 128.f + 127.f * std::sin(elapsed * 2.f);
           pressExitText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(pressExitAlpha)));

           // Draw game over screen
           window.draw(gameOverText);
           window.draw(finalScoreText);
           window.draw(pressExitText);
       } else if (toBeContinued) {
           // To be continued screen animation
           float elapsed = toBeContinuedClock.getElapsedTime().asSeconds();
           
           // Fade in to be continued text
           if (toBeContinuedAlpha < 255.f) {
               toBeContinuedAlpha = std::min(255.f, elapsed * 100.f);
               toBeContinuedText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(toBeContinuedAlpha)));
               finalScoreText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(toBeContinuedAlpha)));
           }
           
           // Blink "Press Exit" text
           pressExitAlpha = 128.f + 127.f * std::sin(elapsed * 2.f);
           pressExitText.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(pressExitAlpha)));

           // Draw to be continued screen
           window.draw(toBeContinuedText);
           window.draw(finalScoreText);
           window.draw(pressExitText);
       } else {
           player.playerMovement();  

           // Remove dead enemies that have finished their death animation  
           enemies.remove_if([](Enemy& e) { return !e.isAlive && e.isDeathAnimationComplete(); });  

           // Check if all enemies are dead and load next map if so
           if (areAllEnemiesDead() && enemies.empty()) {
               if (mapManager.getCurrentMapNumber() == 1) {
                   levelCleared = true;
                   levelClearedClock.restart();
                   levelClearedAlpha = 0.f;
                   pressContinueAlpha = 0.f;
               } else if (mapManager.getCurrentMapNumber() == 2) {
                   toBeContinued = true;
                   toBeContinuedClock.restart();
                   toBeContinuedAlpha = 0.f;
                   pressExitAlpha = 0.f;
                   finalScoreText.setString("FINAL SCORE " + std::to_string(player.score));
                   finalScoreText.setPosition(sf::Vector2f(
                       window.getSize().x / 2.f - finalScoreText.getGlobalBounds().size.x / 2.f,
                       window.getSize().y / 2.f
                   ));
                   std::cout << "Level 2 cleared! Showing To Be Continued screen..." << std::endl;
               } else {
                   loadNextMap();
               }
           }

           // Update and check all enemies  
           for (auto& enemy : enemies) {  
               enemy.enemyMovement(playerSprite.getPosition());  
               updateHitBox(player, enemy);  
           }  

           // Heal the player if applicable  
           updateHealing(player);  

           // Check if the player is dead  
           if (player.isDead && player.deathAnimationComplete) {  
               gameOver = true;
               gameOverClock.restart();
               gameOverAlpha = 0.f;
               pressExitAlpha = 0.f;
               finalScoreText.setString("FINAL SCORE " + std::to_string(player.score));
               finalScoreText.setPosition(sf::Vector2f(
                   window.getSize().x / 2.f - finalScoreText.getGlobalBounds().size.x / 2.f,
                   window.getSize().y / 2.f
               ));
           }  

           // Update text content
           scoreText.setString("Score: " + std::to_string(player.score));
           mapText.setString("Level: " + std::to_string(mapManager.getCurrentMapNumber()));

           // Draw the map as background  
           mapManager.draw(window, mapTilesheet);  

           // Draw all enemies  
           for (auto& enemy : enemies) {  
               enemy.draw(window);  
           }  

           window.draw(playerSprite);  

           // Draw the player's health  
           player.playerHealth(window);  

           // Draw text
           window.draw(scoreText);
           window.draw(mapText);

           // Update and draw sword  
           sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));  
           
           sword.updatePosition(playerSprite.getPosition(), playerSprite.getGlobalBounds().size);  
           sword.updateSwing(player.isAttacking);  
           sword.draw(window);  

           // Draw map information  
           drawMapInfo(window, mapManager);  
       }

       window.display();  
   }  

   return 0;  
}