#include "SnakeController.hpp"
#include "SnakeModel.hpp"
#include "SnakeView.hpp"

int main()
{
    constexpr int kWindowWidth = 1280;
    constexpr int kWindowHeight = 720;
    constexpr float kStepInterval = 0.11f; // 110 ms per passo

    SnakeView view(kWindowWidth, kWindowHeight, "Snake MVC - Isometric 3D");
    SnakeModel model;
    SnakeController controller(model, view, kStepInterval);

    controller.run();
    return 0;
}
