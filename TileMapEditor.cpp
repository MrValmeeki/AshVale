#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <optional>
#include <thread>

const int TILE_SIZE = 16; // Base size of each tile in pixels
const int SCALE_FACTOR = 3; // Scale factor for map window only
const int SCALED_TILE_SIZE = TILE_SIZE * SCALE_FACTOR;
const int MAP_WIDTH_PIXELS = 1027; // Width of the map in pixels
const int MAP_HEIGHT_PIXELS = 768; // Height of the map in pixels
const int MAP_WIDTH = MAP_WIDTH_PIXELS / TILE_SIZE; // Width of the map (in tiles)
const int MAP_HEIGHT = MAP_HEIGHT_PIXELS / TILE_SIZE; // Height of the map (in tiles)

class Tile {
public:
    Tile() : textureRect(sf::IntRect({ 0, 0 }, { TILE_SIZE, TILE_SIZE })), isPlaced(false), isPassable(true) {}
    Tile(const sf::IntRect& rect) : textureRect(rect), isPlaced(true), isPassable(true) {}

    const sf::IntRect& getTextureRect() const { return textureRect; }
    void setTextureRect(const sf::IntRect& rect) {
        textureRect = rect;
        isPlaced = true;
    }
    bool hasBeenPlaced() const { return isPlaced; }

    bool isPassableByPlayer() const { return isPassable; }
    void setPassable(bool passable) { isPassable = passable; }

private:
    sf::IntRect textureRect;
    bool isPlaced;
    bool isPassable;
};

class TileMap {
public:
    TileMap(int width, int height) {
        map.resize(height, std::vector<Tile>(width));
    }

    void draw(sf::RenderWindow& window, const sf::Texture& tilesheet) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                const auto& tile = map[y][x];
                if (tile.hasBeenPlaced()) {
                    const auto& tileRect = tile.getTextureRect();
                    sf::Sprite sprite(tilesheet);
                    sprite.setTextureRect(tileRect);
                    sprite.setPosition(sf::Vector2f(static_cast<float>(x * SCALED_TILE_SIZE),
                        static_cast<float>(y * SCALED_TILE_SIZE)));
                    sprite.setScale(sf::Vector2f(SCALE_FACTOR, SCALE_FACTOR));

                    // Draw non-passable tiles with a slight red tint
                    if (!tile.isPassableByPlayer()) {
                        sprite.setColor(sf::Color(255, 200, 200, 255));
                    }

                    window.draw(sprite);
                }
            }
        }
    }

    void setTile(int x, int y, const sf::IntRect& textureRect, bool passable = true) {
        if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
            map[y][x].setTextureRect(textureRect);
            map[y][x].setPassable(passable);
        }
    }

    void save(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (file) {
            for (const auto& row : map) {
                for (const auto& tile : row) {
                    bool placed = tile.hasBeenPlaced();
                    file.write(reinterpret_cast<const char*>(&placed), sizeof(placed));
                    if (placed) {
                        const auto& rect = tile.getTextureRect();
                        file.write(reinterpret_cast<const char*>(&rect.position.x), sizeof(rect.position.x));
                        file.write(reinterpret_cast<const char*>(&rect.position.y), sizeof(rect.position.y));
                        file.write(reinterpret_cast<const char*>(&rect.size.x), sizeof(rect.size.x));
                        file.write(reinterpret_cast<const char*>(&rect.size.y), sizeof(rect.size.y));

                        // Save passable property
                        bool passable = tile.isPassableByPlayer();
                        file.write(reinterpret_cast<const char*>(&passable), sizeof(passable));
                    }
                }
            }
        }
    }

    void load(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (file) {
            for (auto& row : map) {
                for (auto& tile : row) {
                    bool placed;
                    file.read(reinterpret_cast<char*>(&placed), sizeof(placed));
                    if (placed) {
                        sf::IntRect rect;
                        file.read(reinterpret_cast<char*>(&rect.position.x), sizeof(rect.position.x));
                        file.read(reinterpret_cast<char*>(&rect.position.y), sizeof(rect.position.y));
                        file.read(reinterpret_cast<char*>(&rect.size.x), sizeof(rect.size.x));
                        file.read(reinterpret_cast<char*>(&rect.size.y), sizeof(rect.size.y));
                        tile.setTextureRect(rect);

                        // Load passable property if it exists in the file
                        bool passable = true;
                        if (file.peek() != EOF) {
                            file.read(reinterpret_cast<char*>(&passable), sizeof(passable));
                            tile.setPassable(passable);
                        }
                    }
                }
            }
        }
    }

private:
    std::vector<std::vector<Tile>> map;
};

class TileMapEditor {
public:
    TileMapEditor()
        : mapWindow_(sf::VideoMode(sf::Vector2u(MAP_WIDTH_PIXELS, MAP_HEIGHT_PIXELS)), "Map Editor")
        , tilesheetWindow_(sf::VideoMode(sf::Vector2u(512, 512)), "Tilesheet Selector")
        , tileMap_(MAP_WIDTH, MAP_HEIGHT)
        , selectedTileRect_({ 0, 0 }, { TILE_SIZE, TILE_SIZE })
        , showGrid_(true)
        , currentMapNumber_(1)
        , isPassable_(true) {

        mapWindow_.setFramerateLimit(60);
        tilesheetWindow_.setFramerateLimit(60);

        if (!tilesheet_.loadFromFile("D:/code/C++/Project/Source files/Assest/Map/Fantasy/forest_/forest_1.png")) {
            std::cerr << "Failed to load tilesheet!" << std::endl;
        }

        // Create maps directory if it doesn't exist
        std::filesystem::create_directories("maps");

        // Print available maps on startup
        listAvailableMaps();
    }

    void listAvailableMaps() {
        std::cout << "\nAvailable maps:" << std::endl;
        for (const auto& entry : std::filesystem::directory_iterator("maps")) {
            if (entry.path().extension() == ".dat") {
                std::cout << entry.path().filename().string() << std::endl;
            }
        }
        std::cout << "\nControls:" << std::endl;
        std::cout << "Ctrl + S: Save current map" << std::endl;
        std::cout << "Ctrl + L: Load last saved map" << std::endl;
        std::cout << "Ctrl + N: New blank map" << std::endl;
        std::cout << "G: Toggle grid" << std::endl;
        std::cout << "P: Toggle passability for selected tile" << std::endl;
        std::cout << "Type a number and press Enter to load that map (e.g., '18' loads map_18.dat)" << std::endl;
    }

    void loadMap(int mapNumber) {
        std::string filename = "maps/map_" + std::to_string(mapNumber) + ".dat";
        if (std::filesystem::exists(filename)) {
            tileMap_.load(filename);
            currentMapNumber_ = mapNumber;
            std::cout << "Loaded map: " << filename << std::endl;
        }
        else {
            std::cout << "Map " << filename << " does not exist!" << std::endl;
        }
    }

    void run() {
        // Start a thread to handle console input
        std::thread inputThread([this]() {
            while (mapWindow_.isOpen() && tilesheetWindow_.isOpen()) {
                int mapNumber;
                if (std::cin >> mapNumber) {
                    loadMap(mapNumber);
                }
            }
            });
        inputThread.detach();

        while (mapWindow_.isOpen() && tilesheetWindow_.isOpen()) {
            handleEvents();
            update();
            render();
        }
    }

private:
    void handleEvents() {
        // Handle map window events
        while (const std::optional event = mapWindow_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                mapWindow_.close();
                tilesheetWindow_.close();
            }
            else if (event->is<sf::Event::MouseButtonPressed>()) {
                const auto& mouseEvent = *event->getIf<sf::Event::MouseButtonPressed>();
                if (mouseEvent.button == sf::Mouse::Button::Left) {
                    // Calculate tile position based on scaled tile size
                    int tileX = mouseEvent.position.x / (TILE_SIZE * SCALE_FACTOR);
                    int tileY = mouseEvent.position.y / (TILE_SIZE * SCALE_FACTOR);
                    tileMap_.setTile(tileX, tileY, selectedTileRect_, isPassable_);
                }
            }
            else if (event->is<sf::Event::KeyPressed>()) {
                const auto& keyEvent = *event->getIf<sf::Event::KeyPressed>();
                if (keyEvent.code == sf::Keyboard::Key::G) {
                    showGrid_ = !showGrid_;
                }
                else if (keyEvent.code == sf::Keyboard::Key::P) {
                    isPassable_ = !isPassable_;
                    std::cout << "Tile passability set to: " << (isPassable_ ? "Passable" : "Non-passable") << std::endl;
                }
                else if (keyEvent.code == sf::Keyboard::Key::S && keyEvent.control) {
                    // Save with numbered filename
                    std::string filename = "maps/map_" + std::to_string(currentMapNumber_) + ".dat";
                    tileMap_.save(filename);
                    std::cout << "Map saved as: " << filename << std::endl;
                    currentMapNumber_++;
                }
                else if (keyEvent.code == sf::Keyboard::Key::L && keyEvent.control) {
                    // Load the last saved map
                    if (currentMapNumber_ > 1) {
                        loadMap(currentMapNumber_ - 1);
                    }
                }
                // Add N key to create a new blank map
                else if (keyEvent.code == sf::Keyboard::Key::N && keyEvent.control) {
                    tileMap_ = TileMap(MAP_WIDTH, MAP_HEIGHT);
                    std::cout << "Created new blank map" << std::endl;
                }
                // Load specific map number (Ctrl + number)
                else if (keyEvent.control && keyEvent.code >= sf::Keyboard::Key::Num0 && keyEvent.code <= sf::Keyboard::Key::Num9) {
                    int mapNumber = static_cast<int>(keyEvent.code) - static_cast<int>(sf::Keyboard::Key::Num0);
                    loadMap(mapNumber);
                }
            }
        }

        // Handle tilesheet window events
        while (const std::optional event = tilesheetWindow_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                tilesheetWindow_.close();
                mapWindow_.close();
            }
            else if (event->is<sf::Event::MouseButtonPressed>()) {
                const auto& mouseEvent = *event->getIf<sf::Event::MouseButtonPressed>();
                if (mouseEvent.button == sf::Mouse::Button::Left) {
                    int tileX = (mouseEvent.position.x / TILE_SIZE) * TILE_SIZE;
                    int tileY = (mouseEvent.position.y / TILE_SIZE) * TILE_SIZE;
                    selectedTileRect_ = sf::IntRect({ tileX, tileY }, { TILE_SIZE, TILE_SIZE });
                }
            }
        }
    }

    void update() {
        // Update logic here if needed
    }

    void render() {
        // Render map window
        mapWindow_.clear(sf::Color(50, 50, 50));
        tileMap_.draw(mapWindow_, tilesheet_);
        if (showGrid_) {
            drawGrid(mapWindow_);
        }
        mapWindow_.display();

        // Render tilesheet window (no scaling)
        tilesheetWindow_.clear(sf::Color(30, 30, 30));
        sf::Sprite tilesheetSprite(tilesheet_);
        tilesheetWindow_.draw(tilesheetSprite);

        // Draw selection rectangle (no scaling)
        sf::RectangleShape selectionRect;
        selectionRect.setSize(sf::Vector2f(static_cast<float>(TILE_SIZE),
            static_cast<float>(TILE_SIZE)));
        selectionRect.setPosition(sf::Vector2f(static_cast<float>(selectedTileRect_.position.x),
            static_cast<float>(selectedTileRect_.position.y)));
        selectionRect.setFillColor(sf::Color::Transparent);
        selectionRect.setOutlineColor(sf::Color::Red);
        selectionRect.setOutlineThickness(2);
        tilesheetWindow_.draw(selectionRect);

        tilesheetWindow_.display();
    }

    void drawGrid(sf::RenderWindow& window) {
        sf::RectangleShape line(sf::Vector2f(1, window.getSize().y));
        line.setFillColor(sf::Color(100, 100, 100, 100));

        for (unsigned int x = 0; x <= MAP_WIDTH; ++x) {
            line.setPosition(sf::Vector2f(static_cast<float>(x * SCALED_TILE_SIZE), 0));
            window.draw(line);
        }

        line.setSize(sf::Vector2f(window.getSize().x, 1));
        for (unsigned int y = 0; y <= MAP_HEIGHT; ++y) {
            line.setPosition(sf::Vector2f(0, static_cast<float>(y * SCALED_TILE_SIZE)));
            window.draw(line);
        }
    }

    sf::RenderWindow mapWindow_;
    sf::RenderWindow tilesheetWindow_;
    TileMap tileMap_;
    sf::Texture tilesheet_;
    sf::IntRect selectedTileRect_;
    bool showGrid_;
    int currentMapNumber_;
    bool isPassable_;
};

int main() {
    TileMapEditor editor;
    editor.run();
    return 0;
}