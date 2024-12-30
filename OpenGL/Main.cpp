
#include <iostream>  // Include necessary headers
#include <GL/glut.h>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <mutex>
#include <algorithm>
#include <SOIL2.h>
#include <irrKlang.h>
#include <fstream>
#include <string>
#include <sstream>


#define M_PI 3.1415



using namespace std;
using namespace irrklang;

ISoundEngine* engine = createIrrKlangDevice();

struct Player {
    string name;
    int score = 0;
};


bool comparePlayers(const Player& p1, const Player& p2) {
    return p1.score > p2.score; // Sort in descending order
}

void drawbackground() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw dark blue background
    glColor3f(0.0f, 0.0f, 0.2f); // Dark blue color
    glBegin(GL_QUADS);
    glVertex2d(1, -1);
    glVertex2d(1, 1);
    glVertex2d(-1, 1);
    glVertex2d(-1, -1);
    glEnd();

    // Draw white dots for stars
    glColor3f(1.0f, 1.0f, 1.0f); // White color
    glPointSize(2.5f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 2; ++i) {
        float x = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
        float y = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
        glVertex2f(x, y);
    }
    glEnd();

}

// Global variable to track the last shot time
std::chrono::steady_clock::time_point lastShotTime = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point pausedTime = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point pausedTimeStop = std::chrono::steady_clock::now();


vector<Player> sortPlayersByScore(const std::string& logs) {
    std::vector<Player> players;
    std::ifstream file(logs);
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Player player;
        if (iss >> player.name >> player.score) {
            players.push_back(player);
        }
    }

    std::sort(players.begin(), players.end(), comparePlayers);
    return players;
}


/*struct Obstacle {
    vector<pair<float, float>> shape; // The vertices of the obstacle
    float posX=-1; // X position of the obstacle's center
    float posY=-1; // Y position of the obstacle's center
    bool active=false;
};*/
struct Heart {
    float x = -1; // X position of the heart's center
    float y = -1; // Y position of the heart's center
    float size = -1; // Size of the heart
    bool active = false; // Whether the heart is active (visible)
    std::chrono::steady_clock::time_point startTime;
};
struct Star {
    float x = -1; // X position of the heart's center
    float y = -1; // Y position of the heart's center
    float size = -1; // Size of the heart
    bool active = false; // Whether the heart is active (visible)
    std::chrono::steady_clock::time_point startTime;
};
struct Lazer {
    float x;
    float y;
    float size;
    float speed;
    bool active;
};
struct Alien {
    float x;
    float y;
    float size;
    float speed;
    bool active;
};
int score = 0;
int lives = 3;
int multiplier = 1;
int Level = 1;
bool started = false;
bool moveLeft = false;
bool mute = false;
bool isplaying = false;
bool isplaying2 = false;
bool moveRight = false;
bool endgame = false;
bool typing = false;
bool leaderbord = false;
bool howtoplay = false;
bool pause = false;
long long pausespent = 0;
long long prevspent = 0;
string name = "ENTER_NAME";

float spaceshipX = 0.0f; // Initial position of the cannon

//vector<Obstacle> obstacles;
vector<Lazer> lazers;
vector<Alien> aliens;
vector<Player> sortedPlayers = sortPlayersByScore("texts//logs.txt");

Heart heart;
Star star;

mutex aliensMutex;
std::mutex pauseMutex;

void reset() {
    // Clear vectors
    lazers.clear();
    // obstacles.clear();
    aliens.clear();

    // Reset lives, score, multiplier
    lives = 3;
    score = 0;
    multiplier = 1;
    Level = 1;

    // Reset hearts and stars
    heart.active = false;
    star.active = false;
}



void drawSpaceship() {


    // Draw the red triangle (body of the spaceship)
    glColor3f(1.0f, 0.0f, 0.0f); // Red color
    glBegin(GL_TRIANGLES);
    glVertex2f(spaceshipX, -0.85f); // Bottom center
    glVertex2f(spaceshipX - 0.075f, -0.9f); // Top left
    glVertex2f(spaceshipX + 0.075f, -0.9f); // Top right
    glEnd();

    // Draw the blue wings (triangles on the sides)
    glColor3f(0.0f, 0.0f, 1.0f); // Blue color
    glBegin(GL_TRIANGLES);
    glVertex2f(spaceshipX - 0.05f, -0.85f); // Bottom left
    glVertex2f(spaceshipX - 0.075f, -0.9f); // Top left
    glVertex2f(spaceshipX - 0.025f, -0.9f); // Top right
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex2f(spaceshipX + 0.05f, -0.85f); // Bottom right
    glVertex2f(spaceshipX + 0.025f, -0.9f); // Top left
    glVertex2f(spaceshipX + 0.075f, -0.9f); // Top right
    glEnd();
}
void generateStar() {
    std::default_random_engine generator(std::random_device{}());
    std::uniform_real_distribution<float> xDistribution(-0.9f, 0.9f);
    std::uniform_real_distribution<float> yDistribution(-0.7f, 0.7f);

    while (true) {
        // Check if the game is paused
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            if (pause) {
                continue; // Skip generating stars if the game is paused
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));

        star.x = xDistribution(generator);
        star.y = yDistribution(generator);
        star.size = 0.05f; // Standard size
        star.active = true;
        star.startTime = std::chrono::steady_clock::now();

        // Display the star for 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(10));
        star.active = false;
    }
}


void drawStar() {
    // Check if the star is active before drawing
    if (star.active) {
        glColor3f(1.0, 1.0, 0.0); // Set the color to yellow (or any desired color)
        glBegin(GL_POLYGON); // Begin drawing

        for (int i = 0; i < 8; ++i) {
            double angle = 2 * M_PI * i / 8 - M_PI / 2;
            double x;
            double y;
            if (i % 2 == 0) { // Outer points
                x = star.size * cos(angle) + star.x;
                y = star.size * sin(angle) + star.y;
            }
            else { // Inner points with innerRadiusRatio applied
                x = 0.6 * star.size * cos(angle) + star.x;
                y = 0.6 * star.size * sin(angle) + star.y;
            }
            glVertex2d(x, y);
        }

        glEnd(); // End drawing
        glFlush(); // Flush the buffer
    }
}


void checkAndApplyStarCollision() {
    if (star.active) {
        for (auto it = lazers.begin(); it != lazers.end();) {
            if (it->active && it->x >= star.x - star.size && it->x <= star.x + star.size && it->y >= star.y - star.size && it->y <= star.y + star.size) {
                multiplier++; // Increase lives
                star.active = false; // Deactivate star
                it->active = false; // Deactivate the laser
                if (!mute)
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\saved.mp3", false);
            }
            else {
                ++it;
            }
        }
    }
}
void generateHeart() {
    std::default_random_engine generator(std::random_device{}());
    std::uniform_real_distribution<float> xDistribution(-0.9f, 0.9f);
    std::uniform_real_distribution<float> yDistribution(-0.7f, 0.7f);

    while (true) {
        // Check if the game is paused
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            if (pause) {
                continue; // Skip generating hearts if the game is paused
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));

        heart.x = xDistribution(generator);
        heart.y = yDistribution(generator);
        heart.size = 0.05f; // Standard size
        if (lives < 4)
            heart.active = true;
        heart.startTime = std::chrono::steady_clock::now();

        // Display the heart for 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(10));
        heart.active = false;
    }
}


void drawHeart() {

    if (heart.active) {
        glColor3f(1.0f, 0.0f, 0.0f); // Red color for heart
        glBegin(GL_POLYGON);

        for (float theta = 0.0f; theta <= 2 * M_PI; theta += 0.01f) {
            // Reduce the scaling factor to shrink the heart
            float scale = 0.0025f; // Experiment with different values

            float x = heart.x + scale * (16 * pow(sin(theta), 3));
            float y = heart.y + scale * (13 * cos(theta) - 5 * cos(2 * theta) - 2 * cos(3 * theta) - cos(4 * theta));
            glVertex2f(x, y);
        }

        glEnd();
    }
}

void checkAndApplyHeartCollision() {
    if (heart.active) {
        for (auto it = lazers.begin(); it != lazers.end();) {
            if (it->active && it->x >= heart.x - heart.size && it->x <= heart.x + heart.size && it->y >= heart.y - heart.size && it->y <= heart.y + heart.size) {
                lives++; // Increase lives
                heart.active = false; // Deactivate heart
                it->active = false; // Deactivate the laser
                if (!mute)
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\saved.mp3", false);
            }
            else {
                ++it;
            }
        }
    }
}
/*
void generateObstacle() {

        default_random_engine generator(chrono::system_clock::now().time_since_epoch().count());
        uniform_int_distribution<int> delayDistribution(3, 7);
        uniform_real_distribution<float> xDistribution(-0.9f, 0.9f);
        uniform_real_distribution<float> yDistribution(-0.7f, 0.7f);
        uniform_int_distribution<int> shapeTypeDistribution(1, 4); // Add one more shape type
        uniform_real_distribution<float> sizeDistribution(0.05f, 0.1f);

        while (true) {
            int delay = delayDistribution(generator);
            this_thread::sleep_for(chrono::seconds(delay));

            Obstacle newObstacle;
            newObstacle.posX = xDistribution(generator);
            newObstacle.posY = yDistribution(generator);

            int shapeType = shapeTypeDistribution(generator);
            float size = sizeDistribution(generator);
            newObstacle.active = true;

            switch (shapeType) {
            case 1: { // Triangle
                for (int i = 0; i < 3; i++) {
                    float angle = static_cast<float>(i) / 3 * 2 * M_PI;
                    float vx = newObstacle.posX + size * cos(angle);
                    float vy = newObstacle.posY + size * sin(angle);
                    newObstacle.shape.push_back(make_pair(vx, vy));
                }
                break;
            }
            case 2: { // Square
                newObstacle.shape.push_back(make_pair(newObstacle.posX - size, newObstacle.posY - size));
                newObstacle.shape.push_back(make_pair(newObstacle.posX + size, newObstacle.posY - size));
                newObstacle.shape.push_back(make_pair(newObstacle.posX + size, newObstacle.posY + size));
                newObstacle.shape.push_back(make_pair(newObstacle.posX - size, newObstacle.posY + size));
                break;
            }
            case 3: { // Circle
                int numVertices = 20;
                for (int i = 0; i < numVertices; i++) {
                    float angle = static_cast<float>(i) / numVertices * 2 * M_PI;
                    float vx = newObstacle.posX + size * cos(angle);
                    float vy = newObstacle.posY + size * sin(angle);
                    newObstacle.shape.push_back(make_pair(vx, vy));
                }
                break;
            }


            }
            obstacles.push_back(newObstacle);
        }
}

void drawObstacles() {

        for (const auto& obstacle : obstacles) {
            if (obstacle.active) {

                glColor3f(0.2f, 0.8f, 0.3f); // White color
                glBegin(GL_POLYGON);
                for (const auto& vertex : obstacle.shape) {
                    glVertex2f(vertex.first, vertex.second);
                }
                glEnd();
            }
        }
}
float calculateObstacleSize(const Obstacle& obstacle) {
    float maxSize = 0.0f;

    if (obstacle.shape.size() < 2) {
        return 0.0f; // Return 0 if there are not enough points
    }

    for (size_t i = 0; i < obstacle.shape.size() - 1; ++i) {
        for (size_t j = i + 1; j < obstacle.shape.size(); ++j) {
            float dist = sqrt(pow(obstacle.shape[i].first - obstacle.shape[j].first, 2) +
                pow(obstacle.shape[i].second - obstacle.shape[j].second, 2));
            if (dist > maxSize) {
                maxSize = dist;
            }
        }
    }

    return maxSize;
}


*/


void generateAliens() {
    default_random_engine generator(chrono::system_clock::now().time_since_epoch().count());
    uniform_real_distribution<float> xDistribution(-1.0f, 1.0f);
    uniform_real_distribution<float> speedDistribution(0.001f, 0.003f);

    while (true) {
        // Check if the game is paused
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            if (pause) {
                continue; // Skip generating aliens if the game is paused
            }
        }

        Alien newAlien;
        newAlien.x = xDistribution(generator);
        newAlien.y = 1.0f; // Start from the top
        newAlien.size = 0.13f; // Set alien size
        float baseSpeed = speedDistribution(generator);
        newAlien.speed = baseSpeed * max(Level - 1, 1);
        newAlien.active = true;

        // Lock the mutex before modifying the aliens vector
        aliensMutex.lock();
        aliens.push_back(newAlien);
        aliensMutex.unlock();

        std::this_thread::sleep_for(chrono::seconds(max(10 - Level - 1, 1)));
    }
}

void drawAliens() {
    glColor3f(0.5f, 0.5f, 0.5f); // Gray color for the alien body
    for (const auto& alien : aliens) {
        if (alien.active) {
            // Draw the alien body as a very oval shape using approximation with triangles
            float radiusX = alien.size / 2.0f; // Width radius
            float radiusY = radiusX * 0.5f; // Height radius (adjusted for very oval shape)
            int segments = 30; // Adjust for smoother ovals (more segments, smoother)
            glBegin(GL_TRIANGLE_FAN);
            for (int i = 0; i <= segments; i++) {
                float angle = 2.0f * M_PI * (float)i / segments;
                // Exaggerate the cosine term for a more pointed oval shape
                float x = alien.x + radiusX * cos(angle) * 1.2f;
                float y = alien.y + radiusY * sin(angle);
                glVertex2f(x, y);
            }
            glEnd();

            // Draw the green cockpit (assuming a smaller oval in the center, positioned higher)
            float cockpitRadiusX = alien.size * 0.2f; // Adjust size as needed
            float cockpitRadiusY = cockpitRadiusX * 0.6f; // Maintain oval shape for cockpit
            float cockpitOffset = radiusY * 0.25f; // Adjust for desired height offset (positive value moves up)
            glColor3f(0.0f, 0.8f, 1.0f); // Green color for cockpit
            glBegin(GL_TRIANGLE_FAN);
            for (int i = 0; i <= segments; i++) {
                float angle = 2.0f * M_PI * (float)i / segments;
                float x = alien.x + cockpitRadiusX * cos(angle) / 2.0f; // Adjusted for centering
                float y = alien.y + cockpitOffset + cockpitRadiusY * sin(angle) / 2.0f; // Adjusted for centering and offset
                glVertex2f(x, y);
            }
            glEnd();
            glColor3f(0.5f, 0.5f, 0.5f);
        }

    }
}




void updateAliens() {

    for (auto& alien : aliens) {
        if (alien.active) {
            alien.y -= alien.speed;
            if (alien.y < -1.0f) {
                alien.active = false;
                lives--; // Deactivate alien if it goes off-screen
                if (!mute)
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\explotion.mp3", false);
            }
            if (alien.y <= -0.85f && alien.x >= spaceshipX - 0.15f && alien.x <= spaceshipX + 0.15f) {
                // Collision detected with spaceship
                lives--; // Decrement lives
                alien.active = false; // Deactivate alien
                if (!mute)
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\explotion.mp3", false);
            }
        }
    }

}

void drawLazers() {
    glColor3f(1.0f, 0.8f, 0.0f);// Red color for lasers
    for (const auto& Lazer : lazers) {
        if (Lazer.active) {
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glVertex2f(Lazer.x, Lazer.y);
            glVertex2f(Lazer.x, Lazer.y + Lazer.size);
            glEnd();
            glLineWidth(1.0f);
        }
    }

}

void updateLasers() {
    for (auto& Lazer : lazers) {
        if (Lazer.active) {
            Lazer.y += Lazer.speed;
            if (Lazer.y > 0.99f) {
                Lazer.active = false;// Deactivate laser if it goes off-screen
            }
        }
    }

}
/*
bool checkCollision( Obstacle& obstacle,Lazer& lazer) {
    if (obstacle.active&&lazer.active) {
        float obstacleSize = calculateObstacleSize(obstacle);
        // Calculate the distance between the center of the obstacle and the lazer
        float distanceX = abs(lazer.x - obstacle.posX);
        float distanceY = abs(lazer.y - obstacle.posY);

        // Clamp the distance to be within the bounds of the obstacle
        float closestX = std::max(obstacle.posX - obstacleSize / 2, std::min(lazer.y, obstacle.posX + obstacleSize / 2));
        float closestY = std::max(obstacle.posY - obstacleSize / 2, std::min(lazer.y, obstacle.posY + obstacleSize / 2));

        // Calculate the distance between the closest point on the obstacle and the lazer
        float distanceSquared = (closestX - lazer.y) * (closestX - lazer.x) + (closestY - lazer.y) * (closestY - lazer.y);

        // Check if the distance is less than the lazer's size
        return distanceSquared <= lazer.size * lazer.size;
    }
    return false;
}

bool checkCollision(Obstacle& obstacle, Alien& alien) {
    if (obstacle.active && alien.active) {
        float obstacleSize = calculateObstacleSize(obstacle);
        // Calculate the distance between the center of the obstacle and the alien
        float distanceX = abs(alien.x - obstacle.posX);
        float distanceY = abs(alien.y - obstacle.posY);

        // Clamp the distance to be within the bounds of the obstacle
        float closestX = std::max(obstacle.posX - obstacleSize / 2, std::min(alien.y, obstacle.posX + obstacleSize / 2));
        float closestY = std::max(obstacle.posY - obstacleSize / 2, std::min(alien.y, obstacle.posY + obstacleSize / 2));

        // Calculate the distance between the closest point on the obstacle and the alien
        float distanceSquared = (closestX - alien.y) * (closestX - alien.x) + (closestY - alien.y) * (closestY - alien.y);

        // Check if the distance is less than the alien's size
        return distanceSquared <= alien.size * alien.size;
    }
    return false;
}


void checkAndRemoveCollisions() {
    // Check collisions for regular lazers
    for (auto LazerIt = lazers.begin(); LazerIt != lazers.end();) {
        bool collisionDetected = false;
        for (auto obstacleIt = obstacles.begin(); obstacleIt != obstacles.end() && !collisionDetected; ++obstacleIt) {
            if (obstacleIt->active && checkCollision(*obstacleIt,*LazerIt)) {
                // Collision detected
                collisionDetected = true;
                obstacleIt->active = false;
                LazerIt->active = false;

            }
        }
            ++LazerIt;

    }



    // Check collisions for aliens
    aliensMutex.lock();
    for (auto AlienIt = aliens.begin(); AlienIt != aliens.end();) {
        bool collisionDetected = false;
        for (auto obstacleIt = obstacles.begin(); obstacleIt != obstacles.end() && !collisionDetected;) {
            if (checkCollision(*obstacleIt, *AlienIt)) {
                // Collision detected
                collisionDetected = true;
                obstacleIt->active = false;
            }
            else {
                ++obstacleIt;
            }
        }
        if (collisionDetected) {
            AlienIt->active = false;

        }
        else {
            ++AlienIt;
        }
    }
    aliensMutex.unlock();
}
*/


void checkAndRemoveAlienCollisions() {
    for (int i = 0; i < lazers.size(); i++) {
        if (!lazers[i].active) continue; // Skip inactive lazers
        for (int j = 0; j < aliens.size(); j++) {
            if (!aliens[j].active) continue; // Skip inactive aliens

            // Calculate distance between lazer and alien
            float distance = sqrt(pow(lazers[i].x - aliens[j].x, 2) + pow(lazers[i].y - aliens[j].y, 2));
            if (distance < lazers[i].size - 0.07 + aliens[j].size) {
                lazers[i].active = false; // Deactivate lazer
                aliens[j].active = false; // Deactivate alien
                score = 100 * multiplier + score; // Increase score
                if (!mute && engine) { // Check if engine is initialized
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\explotion.mp3", false);
                }
                break; // No need to check further for this lazer
            }
        }
    }
}




void drawText(const char* text, float x, float y) {
    glColor3f(1.0, 1.0, 1.0); // Set text color to white
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
        ++text;
    }
}

void drawHearts(int lives) {
    glColor3f(1.0f, 0.0f, 0.0f); // Set color to red
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(340, 0, 650, -35); // Assuming window size is 800x600

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (int i = 0; i < lives; i++) {
        glBegin(GL_POLYGON);
        for (float theta = 0.0f; theta < 2 * M_PI; theta += 0.1f) {
            float R = 0.5f; // Adjust size as needed
            float x = R * 16 * pow(sin(theta), 3);
            float y = -R * (13 * cos(theta) - 5 * cos(2 * theta) - 2 * cos(3 * theta) - cos(4 * theta)); // Negate y to flip the heart
            glVertex2f(25 + x + 25 * i, 25 + y); // Adjust position as needed
        }
        glEnd();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}



void display() {
    if (started && !endgame && !leaderbord && !howtoplay) {
        if (lives <= 0) {
            endgame = true;
            return;
        }
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();


        if (pause) {
            pausedTimeStop = now;
            auto pausedElapsed = std::chrono::duration_cast<std::chrono::seconds>(pausedTimeStop - pausedTime).count();
            cout << pausedElapsed;
            pausespent = pausedElapsed;

        }
        elapsed -= pausespent;
        if (!pause) {

            //checkAndRemoveCollisions();
            checkAndRemoveAlienCollisions();
            drawbackground();

            //drawObstacles();
            drawSpaceship();
            drawLazers();
            updateLasers();
            drawHeart();
            checkAndApplyHeartCollision();
            drawStar();
            checkAndApplyStarCollision();
            aliensMutex.lock();
            drawAliens();
            updateAliens();
            drawHearts(lives);

            aliensMutex.unlock();




            if (elapsed > 0 && elapsed < 270 && elapsed % 30 == 0) {
                glColor3f(1.0f, 1.0f, 1.0f);
                char lev[] = "LEVEL ";
                glRasterPos2f(-0.15f, 0.15f);
                for (int i = 0; lev[i] != '\0'; ++i) {
                    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, lev[i]);
                }
                glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, to_string(Level)[0]);
                if (Level == 9) {
                    char end[] = "ENDLESS";
                    glRasterPos2f(-0.15f, 0.05f);
                    for (int i = 0; end[i] != '\0'; ++i) {
                        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, end[i]);
                    }
                }
            }
            if (elapsed > 0 && elapsed < 270 && !mute) {
                // Calculate the current 30-second interval
                int currentInterval = elapsed / 30;

                // Check if a new interval has started and the sound is not already playing
                if (elapsed % 30 == 0 && !isplaying2) {
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\levelup.mp3", false);
                    isplaying2 = true; // Set the flag to true to indicate that the sound is playing
                }
                else if (elapsed % 30 != 0) {
                    isplaying2 = false; // Reset the flag if elapsed is not a multiple of 30
                }
            }
            char timeStr[100];
            sprintf_s(timeStr, "Time: %lld seconds", elapsed);
            drawText(timeStr, -0.95f, 0.9f);
            if (Level < 9)
                Level = elapsed / 30 + 1; // Increment the level every minute
            char levelStr[50];
            sprintf_s(levelStr, "Level: %d", Level);
            drawText(levelStr, -0.95f, 0.8f);
            char scoreStr[50];
            sprintf_s(scoreStr, "Score: %d", score);
            drawText(scoreStr, 0.5f, 0.9f);
            char multStr[50];
            sprintf_s(multStr, "Multiplier: %d", multiplier);
            drawText(multStr, 0.5f, 0.7f);
            if (mute) {
                engine->stopAllSounds();
                isplaying = false;
            }
            if (!isplaying) {
                if (!mute) {
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\music.mp3", true);
                    isplaying = true;
                }
            }
            glutSwapBuffers();
        }
        else {
            drawbackground();
            char pause[] = "PAUSE";
            glRasterPos2f(-0.15f, 0.6f);
            for (int i = 0; pause[i] != '\0'; ++i) {
                glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, pause[i]);
            }
            glColor3f(0.0f, 1.0f, 0.0f); // Red color
            glBegin(GL_QUADS);
            glVertex2f(-0.2f, -0.05f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
            glVertex2f(0.2f, -0.05f);
            glVertex2f(0.2f, -0.15f);    // Adjusted Y coordinates to set the height of the button
            glVertex2f(-0.2f, -0.15f);
            glEnd();
            glColor3f(0.0f, 0.0f, 0.0f);
            char resumeText[] = "Resume";
            glRasterPos2f(-0.125f, -0.12f);
            for (int i = 0; resumeText[i] != '\0'; ++i) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, resumeText[i]);
            }
            glColor3f(1.0f, 0.0f, 0.0f); // Red color
            glBegin(GL_QUADS);
            glVertex2f(-0.2f, -0.25f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
            glVertex2f(0.2f, -0.25f);
            glVertex2f(0.2f, -0.35f);    // Adjusted Y coordinates to set the height of the button
            glVertex2f(-0.2f, -0.35f);
            glEnd();
            glColor3f(0.0f, 0.0f, 0.0f);
            char menuText[] = "Menu";
            glRasterPos2f(-0.1f, -0.32f);
            for (int i = 0; menuText[i] != '\0'; ++i) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, menuText[i]);
            }
            glutSwapBuffers();
        }
    }
    else if (!started && !endgame && !leaderbord && !howtoplay) {
        if (mute) {
            engine->stopAllSounds();
            isplaying = false;
        }
        if (!isplaying) {
            if (!mute) {
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\music.mp3", true);
                isplaying = true;
            }


        }
        drawbackground();
        glColor3f(1.0f, 1.0f, 0.8f);
        char gamen[] = "SPACE";
        glRasterPos2f(-0.05f, 0.67f);
        for (int i = 0; gamen[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, gamen[i]);
        }
        char gamef[] = "SHOOTER";
        glRasterPos2f(-0.2f, 0.6f);
        for (int i = 0; gamef[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, gamef[i]);
        }

        glColor3f(1.0f, 1.0f, 0.8f); // Black color
        char nameText[] = "Name:";
        glRasterPos2f(-0.75f, 0.18f);
        for (int i = 0; nameText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, nameText[i]);
        }

        // Draw name input button
        glColor3f(1.0f, 1.0f, 0.8f); // Green color
        glBegin(GL_QUADS);
        glVertex2f(-0.5f, 0.15f);   // Adjusted Y coordinates to make the button smaller
        glVertex2f(0.5f, 0.15f);
        glVertex2f(0.5f, 0.25f);
        glVertex2f(-0.5f, 0.25f);
        glEnd();

        glColor3f(0.0f, 0.0f, 0.0f); // Black color
        glRasterPos2f(-0.45f, 0.18f);
        for (int i = 0; i < name.length(); ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, name[i]);
        }

        // Draw start button
        glColor3f(0.0f, 1.0f, 0.0f); // Green color
        glBegin(GL_QUADS);
        glVertex2f(-0.2f, -0.05f);   // Adjusted Y coordinates to make the button smaller
        glVertex2f(0.2f, -0.05f);
        glVertex2f(0.2f, 0.05f);
        glVertex2f(-0.2f, 0.05f);
        glEnd();

        // Draw leaderboard button
        glColor3f(1.0f, 0.0f, 0.0f); // Red color
        glBegin(GL_QUADS);
        glVertex2f(-0.2f, -0.25f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
        glVertex2f(0.2f, -0.25f);
        glVertex2f(0.2f, -0.15f);    // Adjusted Y coordinates to set the height of the button
        glVertex2f(-0.2f, -0.15f);
        glEnd();

        //draw how to play button
        glColor3f(1.0f, 0.0f, 0.0f); // Red color
        glBegin(GL_QUADS);
        glVertex2f(-0.2f, -0.45f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
        glVertex2f(0.2f, -0.45f);
        glVertex2f(0.2f, -0.35f);    // Adjusted Y coordinates to set the height of the button
        glVertex2f(-0.2f, -0.35f);
        glEnd();

        //draw mute button
        glColor3f(1.0f, 0.0f, 0.0f); // Red color
        glBegin(GL_QUADS);
        glVertex2f(-0.2f, -0.65f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
        glVertex2f(0.2f, -0.65f);
        glVertex2f(0.2f, -0.55f);    // Adjusted Y coordinates to set the height of the button
        glVertex2f(-0.2f, -0.55f);
        glEnd();

        // Draw diagonal line
        if (mute) {
            glColor3f(0.0f, 0.0f, 0.0f); // Green color
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glVertex2f(-0.2f, -0.55f);   // Diagonal line from bottom-left to top-right
            glVertex2f(0.2f, -0.65f);
            glEnd();
        }


        //draw exit button
        glColor3f(1.0f, 0.0f, 0.0f); // Red color
        glBegin(GL_QUADS);
        glVertex2f(-0.2f, -0.85f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
        glVertex2f(0.2f, -0.85f);
        glVertex2f(0.2f, -0.75f);    // Adjusted Y coordinates to set the height of the button
        glVertex2f(-0.2f, -0.75f);
        glEnd();


        // Draw text on start button
        glColor3f(0.0f, 0.0f, 0.0f); // Black color
        char startText[] = "Start";
        glRasterPos2f(-0.08f, -0.02f);
        for (int i = 0; startText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, startText[i]);
        }

        char LeaderText[] = "Ranks";
        glRasterPos2f(-0.08f, -0.22f);
        for (int i = 0; LeaderText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, LeaderText[i]);
        }

        char howText[] = "How to play";
        glRasterPos2f(-0.2f, -0.42f);
        for (int i = 0; howText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, howText[i]);
        }

        char muteText[] = "Mute";
        glRasterPos2f(-0.08f, -0.62f);
        for (int i = 0; muteText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, muteText[i]);
        }

        char exitText[] = "Exit";
        glRasterPos2f(-0.08f, -0.82f);
        for (int i = 0; exitText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, exitText[i]);
        }

        glutSwapBuffers();
    }
    else if (endgame) {
        drawbackground();
        // Draw game over text
        glColor3f(1.0f, 1.0f, 1.0f); // Red color
        glRasterPos2f(-0.225f, 0.5f);
        const char* gameoverStr = "Game Over!";
        for (int i = 0; gameoverStr[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, gameoverStr[i]);
        }
        glRasterPos2f(-0.225f, 0.15f);
        const char* scr = "score : ";
        for (int i = 0; scr[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, scr[i]);
        }
        for (int i = 0; to_string(score)[i] != '\0'; i++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, to_string(score)[i]);
        }
        if (mute) {
            engine->stopAllSounds();
            isplaying = false;
        }
        if (!isplaying) {
            if (!mute) {
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\music.mp3", true);
                isplaying = true;
            }


        }


        // Draw restart button
        //draw mute button
        glColor3f(0.0f, 1.0f, 0.0f); // Red color
        glBegin(GL_QUADS);
        glVertex2f(-0.2f, -0.05f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
        glVertex2f(0.2f, -0.05f);
        glVertex2f(0.2f, -0.15f);    // Adjusted Y coordinates to set the height of the button
        glVertex2f(-0.2f, -0.15f);
        glEnd();
        glColor3f(0.0f, 0.0f, 0.0f);
        char muteText[] = "Return";
        glRasterPos2f(-0.1f, -0.12f);
        for (int i = 0; muteText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, muteText[i]);
        }
        glutSwapBuffers();

    }
    else if (leaderbord) {
        drawbackground();
        glColor3f(0.0f, 1.0f, 0.0f); // Red color
        glBegin(GL_QUADS);
        glVertex2f(-0.2f, -0.65f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
        glVertex2f(0.2f, -0.65f);
        glVertex2f(0.2f, -0.75f);    // Adjusted Y coordinates to set the height of the button
        glVertex2f(-0.2f, -0.75f);
        glEnd();
        glColor3f(0.0f, 0.0f, 0.0f);
        char muteText[] = "Return";
        glRasterPos2f(-0.1f, -0.72f);
        for (int i = 0; muteText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, muteText[i]);
        }
        if (mute) {
            engine->stopAllSounds();
            isplaying = false;
        }
        if (!isplaying) {
            if (!mute) {
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\music.mp3", true);
                isplaying = true;
            }


        }

        glColor3f(1.0f, 1.0f, 1.0f);
        char lText[] = "Ranks :";
        glRasterPos2f(-0.9f, 0.7f);
        for (int i = 0; lText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, lText[i]);
        }
        float y = 0.5;
        for (int j = 0; j < 9; j++) {
            glRasterPos2f(-0.7f, y);
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, to_string(j + 1)[0]);
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, '-');

            for (int i = 0; i < sortedPlayers[j].name.size(); ++i) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, sortedPlayers[j].name[i]);
            }
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ' ');
            for (int l = 0; l < to_string(sortedPlayers[j].score).size(); ++l) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, to_string(sortedPlayers[j].score)[l]);
            }
            y -= 0.1;

        }

        glutSwapBuffers();

    }
    else if (howtoplay) {

        if (mute) {
            engine->stopAllSounds();
            isplaying = false;
        }
        if (!isplaying) {
            if (!mute) {
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\music.mp3", true);
                isplaying = true;
            }


        }


        drawbackground();

        glColor3f(0.0f, 1.0f, 0.0f); // Red color
        glBegin(GL_QUADS);
        glVertex2f(-0.2f, -0.65f);   // Adjusted Y coordinates to make the button smaller and position it below the start button
        glVertex2f(0.2f, -0.65f);
        glVertex2f(0.2f, -0.75f);    // Adjusted Y coordinates to set the height of the button
        glVertex2f(-0.2f, -0.75f);
        glEnd();
        glColor3f(0.0f, 0.0f, 0.0f);
        char muteText[] = "Return";
        glRasterPos2f(-0.1f, -0.72f);
        for (int i = 0; muteText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, muteText[i]);
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        char hText[] = "How to play :";
        glRasterPos2f(-0.9f, 0.7f);
        for (int i = 0; hText[i] != '\0'; ++i) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, hText[i]);
        }
        fstream how("texts\\howtoplay.txt");
        if (!how.is_open()) {
            cout << "Failed to open the file." << std::endl;
        }
        string line;
        float y = 0.5f;
        while (getline(how, line)) { // Read each line
            glRasterPos2f(-0.7f, y);
            for (int i = 0; line[i] != '\0'; ++i) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, line[i]);
            }
            y -= 0.1;
        }


        how.close();

        glutSwapBuffers();


    }


}

void timer(int) {
    glutPostRedisplay(); // Trigger a redraw
    glutTimerFunc(16, timer, 0); // 16 milliseconds (approximately 60 frames per second)
}

void specialKeys(int key, int x, int y) {
    if (started) {
        if (key == GLUT_KEY_LEFT)
            moveLeft = true;
        else if (key == GLUT_KEY_RIGHT)
            moveRight = true;

    }

}

void moveSpaceship() {
    while (true) {
        if (moveLeft && spaceshipX >= -1) {
            spaceshipX -= 0.04f; // Move the spaceship to the left
        }
        else if (moveLeft && spaceshipX < -1) {
            spaceshipX = 1.0f; // Wrap around to the right edge
        }

        if (moveRight && spaceshipX <= 1) {
            spaceshipX += 0.04f; // Move the spaceship to the right
        }
        else if (moveRight && spaceshipX > 1) {
            spaceshipX = -1.0f; // Wrap around to the left edge
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // Wait for a short duration
    }
}

void handlereleased(unsigned char key, int x, int y) {

    if (started) {
        if (key == 97)

            moveLeft = false;
        if (key == 100)
            moveRight = false;
    }


}

void handlereleased(int key, int x, int y) {

    if (started) {
        if (key == GLUT_KEY_LEFT)
            moveLeft = false;
        if (key == GLUT_KEY_RIGHT)
            moveRight = false;

    }


}


void mouse(int button, int state, int x, int y) {
    float mouseX = (float)x / glutGet(GLUT_WINDOW_WIDTH) * 2.0f - 1.0f;
    float mouseY = (1.0f - (float)y / glutGet(GLUT_WINDOW_HEIGHT)) * 2.0f - 1.0f;
    if (started && !endgame) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !pause) {
            // Get the current time
            auto now = std::chrono::steady_clock::now();

            // Check if at least x seconds have passed since the last shot
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastShotTime).count() >= 0) {
                // Create a new stickman at the cannon position and add it to the vector
                Lazer newLazer;
                newLazer.x = spaceshipX;
                newLazer.y = -0.85f;
                newLazer.size = 0.03f; // Set stickman size
                newLazer.speed = 0.009f; // Set stickman speed
                newLazer.active = true;
                lazers.push_back(newLazer);
                if (!mute)
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\shot.mp3", false);

                // Update the last shot time to now
                lastShotTime = now;
            }
        }
        else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.15f && mouseY <= -0.05f) {
                pause = false;
            }
            else if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.35f && mouseY <= -0.25f) {

                if (!mute)
                    engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);
                // Start button clicked, start the game
                if (name != "ENTER_NAME") {
                    std::ifstream input("texts/logs.txt");
                    if (!input.is_open()) {
                        std::cerr << "Error opening file!" << std::endl;
                        return;
                    }

                    std::string line;
                    std::stringstream updatedContent;
                    bool found = false;
                    while (getline(input, line)) {
                        size_t pos = line.find(" ");
                        std::string playerName = line.substr(0, pos);
                        if (playerName == name) {
                            found = true;
                            int oldScore = stoi(line.substr(pos + 1));
                            if (score > oldScore) {
                                // Update the score if the new score is higher
                                line = name + " " + std::to_string(score);
                            }
                        }
                        updatedContent << line << std::endl;
                    }
                    input.close();

                    // If the player's name was not found in the file, add it with the current score
                    if (!found) {
                        updatedContent << name + " " + std::to_string(score) << std::endl;
                    }

                    // Write the updated or new scores back to the file
                    std::ofstream logs("texts/logs.txt");
                    if (!logs.is_open()) {
                        std::cerr << "Error opening file!" << std::endl;
                        return;
                    }
                    logs << updatedContent.str();
                    logs.close();
                }
                sortedPlayers = sortPlayersByScore("texts\\logs.txt");
                started = false;
                endgame = false;

            }
        }
    }
    if (!started && !howtoplay && !leaderbord && !endgame && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {

        if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.05f && mouseY <= 0.05f&&name!="ENTER_NAME") {
            // Start button clicked, start the game
            if (typing)
                typing = false;
            reset();
            started = true;
            startTime = std::chrono::steady_clock::now();
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\start.mp3", false);
        }
        else if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.25f && mouseY <= -0.15f) {
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);
            cout << "leaderboard" << endl;
            if (typing)
                typing = false;
            started = false;
            leaderbord = true;
        }
        else if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.65f && mouseY <= -0.55f) {
            if (typing)
                typing = false;
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);
            if (mute)
                mute = false;
            else
                mute = true;
            cout << "mute " << mute << endl;
        }
        else if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.45f && mouseY <= -0.35f) {
            if (typing)
                typing = false;
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);
            cout << "how to play" << endl;
            howtoplay = true;
        }
        else if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.85f && mouseY <= -0.75f) {
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\select.mp3", false);
            glutDestroyWindow(glutGetWindow());
        }
        else if (mouseX >= -0.5f && mouseX <= 0.5f && mouseY >= 0.15f && mouseY <= 0.25f) {
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);
            name.clear();
            cout << "clear" << endl;
            typing = true;
        }
    }
    else if (endgame && !leaderbord && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.15f && mouseY <= 0.05f) {
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);
            // Start button clicked, start the game
            if (name != "ENTER_NAME") {
                std::ifstream input("texts/logs.txt");
                if (!input.is_open()) {
                    std::cerr << "Error opening file!" << std::endl;
                    return;
                }

                std::string line;
                std::stringstream updatedContent;
                bool found = false;
                while (getline(input, line)) {
                    size_t pos = line.find(" ");
                    std::string playerName = line.substr(0, pos);
                    if (playerName == name) {
                        found = true;
                        int oldScore = stoi(line.substr(pos + 1));
                        if (score > oldScore) {
                            // Update the score if the new score is higher
                            line = name + " " + std::to_string(score);
                        }
                    }
                    updatedContent << line << std::endl;
                }
                input.close();

                // If the player's name was not found in the file, add it with the current score
                if (!found) {
                    updatedContent << name + " " + std::to_string(score) << std::endl;
                }

                // Write the updated or new scores back to the file
                std::ofstream logs("texts/logs.txt");
                if (!logs.is_open()) {
                    std::cerr << "Error opening file!" << std::endl;
                    return;
                }
                logs << updatedContent.str();
                logs.close();
            }
            sortedPlayers = sortPlayersByScore("texts\\logs.txt");
            started = false;
            endgame = false;

        }

    }
    else if (leaderbord && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.75f && mouseY <= -0.65f) {
            // Start button clicked, start the game
            leaderbord = false;
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);

        }

    }
    else if (howtoplay && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (mouseX >= -0.2f && mouseX <= 0.2f && mouseY >= -0.75f && mouseY <= -0.65f) {
            // Start button clicked, start the game
            howtoplay = false;
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);

        }

    }
}

void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    if (typing) {
        if (key == 13) { // Enter key
            // Submit the name or perform other actions
            std::cout << "Name submitted: " << name << std::endl;
            if (name == "")
                name = "ENTER_NAME";
            typing = false;
            if (!mute)
                engine->play2D("C:\\Users\\tonys\\source\\repos\\OpenGL\\OpenGL\\sounds\\select.mp3", false);
        }
        else if (key == 8) { // Backspace key
            if (!name.empty()) {
                name.pop_back(); // Remove the last character
            }
        }
        else if (key != 32)
            name.push_back(key); // Add the typed character to the name
    }
    if (started) {
        if (key == 97)
            moveLeft = true;
        else if (key == 100)
            moveRight = true;
        else if (key == 27) {
            if (pause) {
                pausedTimeStop = std::chrono::steady_clock::now();
                pause = false;
            }
            else {
                pausedTime = std::chrono::steady_clock::now();
                pause = true;
            }

        }
    }
    if (key == 109) {
        if (mute)
            mute = false;
        else mute = true;
    }

    glutPostRedisplay(); // Request a redraw to update the display
}
void resize_callback(int width, int height) {
    // Undo resizing by setting the window back to its previous size
    glutReshapeWindow(500, 650);
}


int main(int argc, char** argv) {
    //thread obstacleThread(generateObstacle);
    thread aliensThread(generateAliens);
    thread heartThread(generateHeart);
    thread starThread(generateStar);
    thread moveThread(moveSpaceship);



    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(500, 650);
    glutCreateWindow("Space shooter");
    //glutReshapeFunc(resize_callback);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys); // Register the special keys callback
    glutKeyboardUpFunc(handlereleased);
    glutSpecialUpFunc(handlereleased);



    glutMouseFunc(mouse); // Register the mouse callback
    glutTimerFunc(0, timer, 0); // Start the timer
    glutMainLoop();

    starThread.join();
    heartThread.join();
    //obstacleThread.join();
    moveThread.join();
    aliensThread.join(); // Wait for obstacle thread to finish

    return 0;
}
