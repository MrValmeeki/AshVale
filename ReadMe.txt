# AshVale

A 2D action-adventure game built with SFML where you battle through different environments, fighting various enemies and progressing through levels.

## Game Overview

AshVale is a top-down action game where you play as a character navigating through different maps, battling enemies, and trying to survive. The game features multiple levels, each with unique enemies and environments.

## Features

- **Multiple Levels**: Progress through different maps with unique environments
- **Enemy Types**:
  - Slimes: Basic enemies in the first level
  - Goblins: Stronger enemies in the second level
- **Combat System**:
  - Sword attacks with animation
  - Different enemy attack patterns
  - Health system with healing mechanics
- **Movement System**:
  - Smooth character movement
  - Dash ability for quick escapes
- **Score System**: Earn points by defeating enemies
  - Slimes: 10 points each
  - Goblins: 20 points each
- **Audio System**:
  - Dynamic background music
  - Tension music when enemies detect the player
- **Visual Effects**:
  - Character animations for movement, attacks, and death
  - Enemy animations for different states
  - Smooth transitions between levels

## Controls

- **Movement**:
  - W: Move Up
  - A: Move Left
  - S: Move Down
  - D: Move Right
- **Combat**:
  - Left Mouse Button: Attack with sword
- **Special Abilities**:
  - Left Shift: Dash (with cooldown)
- **Game Flow**:
  - Space: Start game / Continue after level completion
  - Numbers 1-9: Switch maps (for testing)
  - Space: Exit after game over or completion

## Game Progression

1. **Level 1**: Forest environment with slime enemies
2. **Level 2**: Tundra environment with goblin enemies
3. **Final Screen**: "To Be Continued" screen with final score

## Technical Details

- Built with SFML (Simple and Fast Multimedia Library)
- C++ implementation
- Custom pathfinding for enemy AI
- Collision detection system
- State-based animation system

## Development

The game is built using:
- SFML for graphics, audio, and input handling
- Custom map management system
- A* pathfinding for enemy movement
- State machine for game flow and animations

## Credits

- Game developed as a C++ project
- Uses SFML for game development
- Custom assets, artists, and developers from itch.io for characters and environments

## Installation

1. Download the Assets folder, SFML libraries, and the AshVale.exe file
2. Run the executable AshVale.exe

## Requirements

- Windows operating system (tested on Windows 11) 

## Personal Changes

If you want to modify the code or make any changes, download everything 

*** SFML 3.0.0 is a must if you want to modify or change the code ***

- "TileMapEditor.cpp" is for making custom maps
- "MapManager.h" helps load the custom maps into the game
