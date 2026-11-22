#include "SnakeController.hpp"
#include "SnakeModel.hpp"
#include "SnakeView.hpp"

int main()
{
    constexpr int kGridWidth = 25;
    constexpr int kGridHeight = 25;
    constexpr int kCellSize = 28;
    constexpr int kHudHeight = 48;
    constexpr float kStepInterval = 0.12f; // 120 ms per passo

    SnakeModel model(kGridWidth, kGridHeight);
    SnakeView view(kCellSize, kHudHeight, "Snake MVC - Raylib + C++");
    SnakeController controller(model, view, kStepInterval);

    controller.run();
    return 0;
}
