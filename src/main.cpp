#include <SDL2/SDL.h>
#include <SDL_render.h>
#include <glm/geometric.hpp>
#include <string>
#include <glm/glm.hpp>
#include <vector>

#include "color.h"
#include "object.h"
#include "sphere.h"
#include "light.h"
#include "camera.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float ASPECT_RATIO = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);

SDL_Renderer* renderer;
std::vector<Object*> objects;
Light light(glm::normalize(glm::vec3(-1, 0.5, 3)), 1.5f, Color(255, 255, 255));

Camera camera(glm::vec3(0, 0, -20.0f), glm::vec3(0, 0, 0), 10.0f);


void point(glm::vec2 position, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawPoint(renderer, position.x, position.y);
}

Color castRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) {
    float zBuffer = 99999;
    Object* hitObject = nullptr;
    Intersect intersect;

    for (const auto& object : objects) {
        Intersect i = object->rayIntersect(rayOrigin, rayDirection);
        if (i.isIntersecting && i.dist < zBuffer) {
            zBuffer = i.dist;
            hitObject = object;
            intersect = i;
        }
    }

    if (!intersect.isIntersecting) {
        return Color(173, 216, 230);
    }

    float diffuseLightIntensity = std::max(0.0f, glm::dot(intersect.normal, light.position));

    Material mat = hitObject->material;

    glm::vec3 lightDir = glm::normalize(light.position - intersect.point);
    glm::vec3 viewDir = glm::normalize(rayOrigin - intersect.point);

    
    // Calculate the reflection direction vector
    // Reflect the negative light direction vector about the normal vector at the intersection point
    glm::vec3 reflectDir = glm::reflect(-lightDir, intersect.normal);
    
    // Calculate the specular light intensity
    // This is done by taking the dot product between the view direction and the reflected light direction,
    // and raising it to the power of the specular coefficient
    float spec = std::pow(std::max(0.0f, glm::dot(viewDir, reflectDir)), mat.specularCoefficient);

    // Calculate the color for the diffuse light
    // Intensity times albedo times color for diffuse light (Lambertian reflection)
    Color diffuseLight = mat.diffuse * light.intensity * diffuseLightIntensity * mat.albedo;

    // Calculate the color for the specular light
    // Intensity times albedo times color for specular light (Phong reflection)
    Color specularLight = light.color * light.intensity * spec * mat.specularAlbedo;

    Color color = diffuseLight + specularLight;
    return color;
} 

void setUp() {
    Material rubber = {
        Color(80, 0, 0),   // diffuse
        0.9f,
        0.1f,
        10.0f
    };

    Material ivory = {
        Color(100, 100, 80),
        0.6f,
        0.3f,
        50.0f
    };


    objects.push_back(new Sphere(glm::vec3(0.0f, 0.0f, -8.0f), 1.0f, rubber));
    objects.push_back(new Sphere(glm::vec3(-1.0f, 0.0f, -4.0f), 1.0f, ivory));
}

void render() {
    float fov = 3.1415/3;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            /* float random_value = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); */
            /* if (random_value > 0.2) { */
                /* continue; */
            /* } */
            float screenX = (2.0f * (x + 0.5f)) / SCREEN_WIDTH - 1.0f;
            float screenY = -(2.0f * (y + 0.5f)) / SCREEN_HEIGHT + 1.0f;
            screenX *= ASPECT_RATIO;
            screenX *= tan(fov/2.0f);
            screenY *= tan(fov/2.0f);

            glm::vec3 cameraDir = glm::normalize(camera.target - camera.position);

            // Compute the real right and up vectors
            glm::vec3 cameraX = glm::normalize(glm::cross(cameraDir, camera.up));
            glm::vec3 cameraY = glm::cross(cameraX, cameraDir);

            glm::vec3 rayDirection = glm::normalize(cameraDir + cameraX * screenX + cameraY * screenY);
            
            Color pixelColor = castRay(camera.position, rayDirection);

            point(glm::vec2(x, y), pixelColor);
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Hello World - FPS: 0", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          SCREEN_WIDTH, SCREEN_HEIGHT, 
                                          SDL_WINDOW_SHOWN);

    if (!window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    int frameCount = 0;
    Uint32 startTime = SDL_GetTicks();
    Uint32 currentTime = startTime;
    
    setUp();

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                            // Move closer to the target
                            camera.move(-1.0f);  // You may need to adjust the value as per your needs
                            break;
                        case SDLK_DOWN:
                            // Move away from the target
                            camera.move(1.0f);  // You may need to adjust the value as per your needs
                            break;
                        case SDLK_a:
                            // Rotate up
                            camera.rotate(-1.0f, 0.0f);  // You may need to adjust the value as per your needs
                            break;
                        case SDLK_d:
                            // Rotate down
                            camera.rotate(1.0f, 0.0f);  // You may need to adjust the value as per your needs
                            break;
                        case SDLK_w:
                            // Rotate left
                            camera.rotate(0.0f, -1.0f);  // You may need to adjust the value as per your needs
                            break;
                        case SDLK_s:
                            // Rotate right
                            camera.rotate(0.0f, 1.0f);  // You may need to adjust the value as per your needs
                            break;
                        default:
                            break;
                    }
            }
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render();

        // Present the renderer
        SDL_RenderPresent(renderer);

        frameCount++;

        // Calculate and display FPS
        if (SDL_GetTicks() - currentTime >= 1000) {
            currentTime = SDL_GetTicks();
            std::string title = "Hello World - FPS: " + std::to_string(frameCount);
            SDL_SetWindowTitle(window, title.c_str());
            frameCount = 0;
        }
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

