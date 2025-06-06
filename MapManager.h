#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

// Constants from TileMapEditor.cpp
const int TILE_SIZE = 16; // Base size of each tile in pixels
const int SCALE_FACTOR = 3; // Scale factor for map window only
const int SCALED_TILE_SIZE = TILE_SIZE * SCALE_FACTOR;
const int MAP_WIDTH_PIXELS = 1280; // Width of the map in pixels
const int MAP_HEIGHT_PIXELS = 720; // Height of the map in pixels
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

                    // No red tint for non-passable tiles
                    sprite.setColor(sf::Color::White);

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

    bool isTilePassable(int x, int y) const {
        if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
            return map[y][x].isPassableByPlayer();
        }
        return false; // Out of bounds is considered non-passable
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

class MapManager {
public:
    MapManager(const std::string& mapsPath = "maps") : currentMap_(MAP_WIDTH, MAP_HEIGHT), mapsDirectory_(mapsPath) {
        // Create maps directory if it doesn't exist
        std::filesystem::create_directories(mapsDirectory_);
    }

    bool loadMap(int mapNumber) {
        std::string filename = mapsDirectory_ + "/map_" + std::to_string(mapNumber) + ".dat";
        if (std::filesystem::exists(filename)) {
            currentMap_.load(filename);
            currentMapNumber_ = mapNumber;
            currentMapFilename_ = filename;
            std::cout << "Loaded map: " << filename << std::endl;
            return true;
        }
        else {
            std::cout << "Map " << filename << " does not exist!" << std::endl;
            return false;
        }
    }

    void draw(sf::RenderWindow& window, const sf::Texture& tilesheet) {
        currentMap_.draw(window, tilesheet);
    }

    void listAvailableMaps() {
        std::cout << "\nAvailable maps:" << std::endl;
        for (const auto& entry : std::filesystem::directory_iterator(mapsDirectory_)) {
            if (entry.path().extension() == ".dat") {
                std::cout << entry.path().filename().string() << std::endl;
            }
        }
    }

    int getCurrentMapNumber() const {
        return currentMapNumber_;
    }

    std::string getCurrentMapFilename() const {
        return currentMapFilename_;
    }

    // Check if a tile at the given position is passable
    bool isTilePassable(int x, int y) const {
        return currentMap_.isTilePassable(x, y);
    }

    // Check if a position is passable (converts from pixel coordinates to tile coordinates)
    bool isPositionPassable(float x, float y) const {
        // Convert from pixel coordinates to tile coordinates
        int tileX = static_cast<int>(x / SCALED_TILE_SIZE);
        int tileY = static_cast<int>(y / SCALED_TILE_SIZE);

        // Check if the position is within the map bounds
        if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT) {
            return false; // Out of bounds is considered non-passable
        }

        return isTilePassable(tileX, tileY);
    }

private:
    TileMap currentMap_;
    int currentMapNumber_ = 1;
    std::string currentMapFilename_;
    std::string mapsDirectory_;
};
