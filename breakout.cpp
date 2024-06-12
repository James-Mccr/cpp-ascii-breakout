#include "lib/console.h"
#include "lib/frame.h"
#include "lib/grid.h"
#include "lib/input.h"
#include "lib/render.h"
#include <chrono>

static constexpr int FPS {60};
static bool GameOver{};
static bool Win{};

class _2DVector
{
public:
    _2DVector(int _x, int _y) : x(_x), y(_y) {}
    int x{};
    int y{};
};

class Block
{
public:
    Block(int _x, int _y) : x(_x), y(_y) {}

    void Update()
    {
        if (idle) return;
        grid.SetTile(x, y, ascii);
        grid.SetTile(x+1, y, ascii);
        grid.SetTile(x+2, y, ascii);
    }

    void SetIdle()
    {
        idle = true;
        grid.SetTile(x, y);
        grid.SetTile(x+1, y);
        grid.SetTile(x+2, y);
    }

    bool IsCollision(int _x, int _y) 
    {
        if (idle) return false;
        return _x >= x && _x <= x+2 && _y == y; 
    }

    static constexpr char ascii = 'B';

private:
    int x{};
    int y{};
    bool idle{};
};

class BlockSpawner
{
public:
    BlockSpawner()
    {
        int x = grid.GetWidth()/9;
        int y = grid.GetHeight()/10;
        for (int x = 3; x < grid.GetWidth()-3; x+=grid.GetWidth()/9)
        {
            blocks.emplace_back(x, y);
            if (y+3+6 >= grid.GetHeight()-2) continue;
            blocks.emplace_back(x, y+3);
            if (y+6+6 >= grid.GetHeight()-2) continue;
            blocks.emplace_back(x, y+6);
            if (y+9+6 >= grid.GetHeight()-2) continue;
            blocks.emplace_back(x, y+9);
        }
        
        for (auto& block : blocks)
            block.Update();
    }

    void Collide(int x, int y)
    {
        for (auto& block : blocks)
        {
            if (!block.IsCollision(x, y)) continue;
            block.SetIdle();
            idleCount++;
            break;
        }

        if (idleCount == blocks.size()) Win = true;
    }

private:
    int idleCount{};
    vector<Block> blocks{};
};

class Player
{
public:
    Player(int _x, int _y)
    {
        x = _x;
        y = _y;
    }

    void Update()
    {
        int _x = x;

        if (userInput == UserInput::Left) x--;
        else if (userInput == UserInput::Right) x++;

        if (grid.IsOutOfBounds(x, y) || grid.IsOutOfBounds(x+4, y)) 
            x = _x;

        for (int i = 0; i < 5; i++)
            grid.SetTile(_x+i, y);

        for (int i = 0; i < 5; i++)
            grid.SetTile(x+i, y, ascii);
            
    }

    int GetX() const { return x; }
    int GetY() const { return y; }

    static constexpr char ascii = '@';

private:
    int x{};
    int y{};
};

class Ball
{
public:
    void Update(const Player& player, BlockSpawner& blockSpawner)
    {
        grid.SetTile(coordinates.x, coordinates.y);

        int _x = coordinates.x;
        momentum.x += speed.x;
        if (momentum.x <= -10 || momentum.x >= 10)
        {
            int offset = momentum.x > 0 ? 1 : -1;
            int xOffset = coordinates.x+offset;
            if (grid.IsOutOfBounds(xOffset, coordinates.y))
            {
                speed.x *= -1;
                offset *= -1;
            }
            else if (grid.IsCollision(xOffset, coordinates.y, Player::ascii))
            {
                momentum.y = -10;
                speed.y = -1;
                speed.x = speed.x > 0 ? -2 : 2;
                offset *= -1;
            }
            else if (grid.IsCollision(xOffset, coordinates.y, Block::ascii))
            {
                speed.x *= -1;
                offset *= -1;
                blockSpawner.Collide(xOffset, coordinates.y);
            }
            coordinates.x += offset;
            momentum.x = 0;
        }

        momentum.y += speed.y;
        if (momentum.y <= -10 || momentum.y >= 10)
        {
            int offset = momentum.y > 0 ? 1 : -1;
            int yOffset = coordinates.y+offset;
            if (yOffset == grid.GetHeight()-1)
            {
                GameOver = true;
                return;
            }
            else if (grid.IsOutOfBounds(coordinates.x, yOffset))
            {
                speed.y = 1;
                offset = 1;
            }
            else if (grid.IsCollision(coordinates.x, yOffset, Player::ascii))
            {
                int barIndex = coordinates.x-player.GetX();
                if (barIndex == 2)
                    speed.x *= -1;
                else
                    speed.x = barIndex-2;
                speed.y = -1;
                coordinates.x = _x;
                offset = -1;
            }
            else if (grid.IsCollision(coordinates.x, yOffset, Block::ascii))
            {
                speed.y *= -1;
                offset *= -1;
                blockSpawner.Collide(coordinates.x, yOffset);
            }
            coordinates.y += offset;
            momentum.y = 0; 
        }

        grid.SetTile(coordinates.x, coordinates.y, ascii);
    }

    static constexpr char ascii = 'o';

private:
    _2DVector coordinates{grid.GetWidth()/2, grid.GetHeight()-4};
    _2DVector speed{1,-1};
    _2DVector momentum{0,0};
};

class Game
{
public:
    void Update()
    {
        player.Update();
        ball.Update(player, blockSpawner);
    }

private:
    Player player{grid.GetWidth()/2, grid.GetHeight()-2};
    Ball ball{};
    BlockSpawner blockSpawner{};
};

int main()
{
    srand(time(NULL));
    Console console{};
    Frame frame{FPS};
    Input input{};
    Render render{console};
    Game game{};

    while(1)
    {
        frame.limit();
        userInput = input.Read();
        if (userInput == UserInput::Quit) return 0;
        game.Update();
        if (GameOver)
        {
            console.moveCursor(grid.GetHeight()/2, grid.GetWidth()/2-5);
            console.print("Game Over");
            break;
        }
        else if (Win)
        {
            render.Draw(grid.GetTiles());
            console.moveCursor(grid.GetHeight()/2, grid.GetWidth()/2-4);
            console.print("You win!");
            break;
        }
        render.Draw(grid.GetTiles());
    }

    frame = {1};
    frame.limit();
    frame.limit();
    frame.limit();
    frame.limit();

    return 0;
}