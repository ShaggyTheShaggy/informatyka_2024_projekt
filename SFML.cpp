#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

const int Window_Height = 1000;
const int Window_Width = 800;
const int Player_Width = 80;
const float Enemy_Width = 0.7f;
const float Enemy_Height = 0.7f;
const int Enemy_Rows = 5;
const int Enemy_Columns = 10;
const int Bullet_Width = 10;
const int Bullet_Height = 30;
const float ENEMY_SPEED = 1.0f;
const float DOWN_STEP = 20.0f;
const int NUM_STARS = 100;  // Liczba gwiazd na ekranie
const float STAR_SPEED = 2.0f;  // Prêdkoœæ poruszania siê gwiazd
const float BULLET_SPEED = 30.0f;
const float ENEMY_BULLET_SPEED = 20.0f;
const int ENEMY_SHOOT_COOLDOWN = 4.0f; // 1% szans na strza³

struct Star {
    sf::CircleShape shape;

    Star(float x, float y, float radius) {
        shape.setRadius(radius);
        shape.setFillColor(sf::Color::White);
        shape.setPosition(x, y);
    }

    void move(float speed) {
        shape.move(0, speed);
        if (shape.getPosition().y > Window_Height) {
            shape.setPosition(shape.getPosition().x, 0);
        }
    }
};

class Explosion {
public:
    sf::CircleShape shape;
    float lifetime;
    bool active;

    Explosion(float x, float y, float radius = 20.0f, float lifetime = 0.5f)
        : lifetime(lifetime), active(true) {
        shape.setRadius(radius);
        shape.setFillColor(sf::Color::Yellow);
        shape.setPosition(x - radius, y - radius);
    }

    void update(float deltaTime) {
        lifetime -= deltaTime;
        if (lifetime <= 0) {
            active = false;
        }
    }
};

class Player {
public:
    sf::Texture player_model;
    sf::Sprite sprite;

    int playerLives = 3;  // Pocz¹tkowa liczba ¿yæ

    Player() {
        if (!player_model.loadFromFile("PlayerModel.png")) {
            std::cout << "Load failed" << std::endl;
            system("pause");
        }
        sprite.setTexture(player_model);
        sprite.setPosition(Window_Width / 2 - 25, Window_Height - 90);
    }

    void move(float dx) {
        float newX = sprite.getPosition().x + dx;
        if (newX >= 0 && newX <= Window_Width - Player_Width) {
            sprite.move(dx, 0);
        }
    }
};

class Enemy {
public:
    sf::Sprite sprite;
    bool active;
    float lastShotTime;  // Czas ostatniego strza³u przeciwnika
    float shootCooldown; // Czas opóŸnienia miêdzy strza³ami

    Enemy(float x, float y, sf::Texture& texture) : active(true), lastShotTime(0.0f) {
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
        sprite.setScale(Enemy_Width, Enemy_Height);

        shootCooldown = ENEMY_SHOOT_COOLDOWN;
    }
    // Funkcja sprawdzaj¹ca, czy przeciwnik mo¿e strzeliæ
    bool canShoot(float currentTime) {
        return (currentTime - lastShotTime >= shootCooldown);
    }

};

// Klasa do przycisków w menu
class MenuButton {
public:
    sf::Sprite sprite;
    bool isHovered = false;

    MenuButton(sf::Texture& texture, float x, float y) {
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
    }

    void checkHover(const sf::Vector2i& mousePos) {
        if (sprite.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
            isHovered = true;
        }
        else {
            isHovered = false;
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(sprite);
    }
};

class Bullet {
public:
    sf::RectangleShape shape;
    bool active;

    Bullet(float x, float y) : active(true) {
        shape.setSize(sf::Vector2f(Bullet_Width, Bullet_Height));
        shape.setFillColor(sf::Color::Red);
        shape.setPosition(x, y);
    }

    void update() {
        shape.move(0, -BULLET_SPEED);
        if (shape.getPosition().y < -1) {
            active = false;
        }
    }
};

struct EnemyBullet {
    sf::RectangleShape shape;

    EnemyBullet(float x, float y) {
        shape.setSize(sf::Vector2f(10, 30));
        shape.setFillColor(sf::Color::Blue);
        shape.setPosition(x, y);
    }

    void moveDown() {
        shape.move(0, ENEMY_BULLET_SPEED);
    }
};

struct PowerUp {
    sf::Sprite sprite;  // Sprite dla power-upa
    sf::Vector2f position;  // Pozycja power-upa
    bool active = false;  // Czy power-up jest aktywny
};

void EnemySpawn(std::vector<Enemy>& enemies, sf::Texture& texture) {
    for (int row = 0; row < Enemy_Rows; ++row) {
        for (int col = 0; col < Enemy_Columns; ++col) {
            float x = col * (Enemy_Width + 60) + 50;
            float y = row * (Enemy_Height + 60) + 50;
            enemies.emplace_back(x, y, texture);
        }
    }
}

void moveEnemies(std::vector<Enemy>& enemies, float& enemySpeedX, bool& moveDown) {
    float leftmostX = Window_Width, rightmostX = 0;

    for (auto& enemy : enemies) {
        sf::Vector2f pos = enemy.sprite.getPosition();
        if (pos.x < leftmostX) leftmostX = pos.x + Enemy_Width;
        if (pos.x + Enemy_Width > rightmostX) rightmostX = pos.x + Enemy_Width;
    }

    if ((leftmostX <= 0 && enemySpeedX < 0) || (rightmostX >= Window_Width && enemySpeedX > 0)) {
        enemySpeedX = -enemySpeedX;
        moveDown = true;
    }

    for (auto& enemy : enemies) {
        enemy.sprite.move(enemySpeedX, moveDown ? DOWN_STEP : 0);
    }
    moveDown = false;
}

void drawHealthBar(sf::RenderWindow& window, int lives) {
    // T³o paska ¿ycia
    sf::RectangleShape background(sf::Vector2f(200, 30));
    background.setFillColor(sf::Color(50, 50, 50));  // Szare t³o
    background.setPosition(Window_Width - 210, 10);  // Pozycja w prawym górnym rogu

    // Pasek ¿ycia
    sf::RectangleShape healthBar(sf::Vector2f(200 * lives / 3.0f, 30));
    healthBar.setFillColor(sf::Color::Green);  // Kolor zielony dla pe³nego ¿ycia
    healthBar.setPosition(Window_Width - 210, 10);  // Pozycja w prawym górnym rogu

    // Rysowanie
    window.draw(background);
    window.draw(healthBar);
}

// Funkcja do rysowania menu
void drawMainMenu(sf::RenderWindow& window, sf::Sprite& background, MenuButton& startButton, MenuButton& exitButton) {
    window.clear();
    window.draw(background);  // T³o menu
    startButton.draw(window);  // Rysowanie przycisku "Start"
    exitButton.draw(window);   // Rysowanie przycisku "Exit"
    window.display();
}

void spawnPowerUp(PowerUp& powerUp, float windowWidth) {
    if (!powerUp.active) {
        powerUp.position.x = rand() % static_cast<int>(windowWidth - 50);  // Losowa pozycja X
        powerUp.position.y = -50;  // Poza górn¹ krawêdzi¹ ekranu
        powerUp.sprite.setPosition(powerUp.position);
        powerUp.active = true;
    }
}

void drawGameOver(sf::RenderWindow& window, sf::Sprite& gameOverSprite) {
    // Rysowanie t³a dla napisu "Game Over"
    window.clear();  // Wyczyœæ ekran
    window.draw(gameOverSprite);  // Rysowanie sprite'a Game Over
    window.display();  // Wyœwietlenie zmienionego ekranu
}

int main() {
    sf::RenderWindow window(sf::VideoMode(Window_Width, Window_Height), "Kosmiczne Pierony");
    window.setFramerateLimit(60);


    Player player;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Explosion> explosions;
    sf::Clock explosionClock;
    float enemySpeedX = ENEMY_SPEED;
    bool moveDown = false;
    bool gameOver = false;  // Flaga "game over"

    int currentWave = 1;  // Która fala obecnie jest
    bool showWaveMessage = false;  
    sf::Clock waveMessageClock;    
    sf::Font waveFont;            
    sf::Text waveText;

    PowerUp powerUp;  // Obiekt power-upa
    float powerUpSpeed = 200.0f;  // Prêdkoœæ opadania power-upa
    bool doubleShotSpeed = false;  // Czy prêdkoœæ strza³ów jest podwojona
    sf::Clock powerUpTimer;  // Zegar do œledzenia czasu dzia³ania power-upa
    float powerUpDuration = 5.0f;  // Czas trwania efektu power-upa (w sekundach)

    sf::Clock shootClock;
    float shootDelay = 0.1f;

    sf::Clock enemyShootClock;
    float enemyShootDelay = 1.0f; // Przeciwnicy strzelaj¹ co sekundê (Zegar przeciwników)

    sf::Clock clock; // Zegar do obliczania czasu
    float deltaTime; // Przechowywanie czasu miêdzy klatkami

    // Load the font for the wave message
    if (!waveFont.loadFromFile("Font.ttf")) {
        std::cout << "Failed to load font!" << std::endl;
        return -1;
    }

    // Set up the wave message text
    waveText.setFont(waveFont);
    waveText.setCharacterSize(50);
    waveText.setFillColor(sf::Color::White);
    waveText.setStyle(sf::Text::Bold);
    waveText.setString("Wave 1");  
    waveText.setPosition(Window_Width / 2 - 100, Window_Height / 2 - 50);


    std::srand(static_cast<unsigned>(std::time(0)));
    std::vector<EnemyBullet> enemyBullets;

    std::vector<Star> stars;
    //t³o gry
    for (int i = 0; i < NUM_STARS; ++i) {
        float x = static_cast<float>(std::rand() % Window_Width);
        float y = static_cast<float>(std::rand() % Window_Height);
        float radius = static_cast<float>(std::rand() % 3 + 1);
        stars.emplace_back(x, y, radius);
    }

    sf::Texture menuBackgroundTexture;
    if (!menuBackgroundTexture.loadFromFile("menuBackground.png")) {
        std::cout << "B³¹d wczytywania t³a menu!" << std::endl;
        return -1;
    }
    sf::Sprite menuBackground(menuBackgroundTexture);

    // Dopasowanie t³a do rozmiaru okna
    menuBackground.setScale(
        static_cast<float>(Window_Width) / menuBackground.getLocalBounds().width,
        static_cast<float>(Window_Height) / menuBackground.getLocalBounds().height
    );

    sf::Texture startButtonTexture;
    if (!startButtonTexture.loadFromFile("startButton.png")) {
        std::cout << "B³¹d wczytywania przycisku 'Start'!" << std::endl;
        return -1;
    }

    sf::Texture exitButtonTexture;
    if (!exitButtonTexture.loadFromFile("exitButton.png")) {
        std::cout << "B³¹d wczytywania przycisku 'Exit'!" << std::endl;
        return -1;
    }

    // Tworzenie przycisków
    MenuButton startButton(startButtonTexture, Window_Width / 2 - 150, Window_Height / 2 - 100);
    MenuButton exitButton(exitButtonTexture, Window_Width / 2 - 150, Window_Height / 2 + 50);

    bool isInMenu = true;

    sf::Texture enemy_model;
    if (!enemy_model.loadFromFile("EnemyModel.png")) {
        std::cout << "Load failed" << std::endl;
        return -1;
    }

    // Wczytujemy obrazek na ekranie "Game Over"
    sf::Texture gameOverTexture;
    if (!gameOverTexture.loadFromFile("GameOverImage.png")) {
        std::cout << "Game Over Image loading failed" << std::endl;
        return -1;
    }

    sf::Sprite gameOverSprite(gameOverTexture);
    gameOverSprite.setPosition(Window_Width / 2 - gameOverSprite.getGlobalBounds().width / 2, Window_Height / 2 - gameOverSprite.getGlobalBounds().height / 2);

    // Za³aduj teksturê dla power-upa
    sf::Texture powerUpTexture;
    if (!powerUpTexture.loadFromFile("PowerUp.png")) {
        // Obs³uga b³êdu ³adowania
    }

    // Inicjalizacja power-upa
    powerUp.sprite.setTexture(powerUpTexture);
    powerUp.sprite.setScale(0.5f, 0.5f);  // Opcjonalnie skalowanie
    powerUp.active = false;



    EnemySpawn(enemies, enemy_model);

    //G³ówna Pêtla
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        deltaTime = clock.restart().asSeconds();

        if (isInMenu) {
            // Sprawdzamy, czy myszka jest nad przyciskami
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            startButton.checkHover(mousePos);
            exitButton.checkHover(mousePos);

            // Obs³uga klikniêæ menu
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (startButton.isHovered) {
                        isInMenu = false; // Rozpoczynamy grê
                    }
                    else if (exitButton.isHovered) {
                        window.close();  // Zamykamy grê
                    }
                }
            }

            drawMainMenu(window, menuBackground, startButton, exitButton);
            continue;  // Przechodzimy do nastêpnej klatki
        }


        if (gameOver) {
            drawGameOver(window, gameOverSprite);  // Wyœwietlenie sprite'a "Game Over"
            continue;  // Przechodzimy do kolejnej klatki bez dalszej logiki gry
        }

        // Check if all enemies are dead
        bool allEnemiesDefeated = std::all_of(enemies.begin(), enemies.end(), [](Enemy& enemy) {
            return !enemy.active;
            });

        if (allEnemiesDefeated) {
            // Move to the next wave
            currentWave++;
            showWaveMessage = true;
            waveText.setString("Wave " + std::to_string(currentWave));
            waveMessageClock.restart();

            // Respawn enemies
            enemies.clear();
            EnemySpawn(enemies, enemy_model);

            // Increase difficulty with each wave
            enemySpeedX += 0.5f; // Szybszy ruch przeciwników
            if (enemyShootDelay > 0.3f) { // Minimalny czas strza³u wynosi 0.3 sekundy
                enemyShootDelay -= 0.2f; // Zmniejsz opóŸnienie strza³u przeciwników
            }
        }

        //LOGIKA POWER'UP
        if (!powerUp.active && (rand() % 500 < 2)) { // Szansa na spawn co klatkê (0.4%)
            spawnPowerUp(powerUp, Window_Width);
        }

        if (powerUp.active) {
            powerUp.position.y += powerUpSpeed * deltaTime;  // Opadanie w dó³
            powerUp.sprite.setPosition(powerUp.position);

            // Dezaktywacja power-upa, jeœli spadnie poni¿ej ekranu
            if (powerUp.position.y > Window_Height) {
                powerUp.active = false;
            }
        }

        if (doubleShotSpeed) {
            shootDelay = 0.05f; // Szybsze strzelanie
        }
        else {
            shootDelay = 0.1f; // Normalne strzelanie
        }

        if (powerUp.active && powerUp.sprite.getGlobalBounds().intersects(player.sprite.getGlobalBounds())) {
            powerUp.active = false;  // Power-up zosta³ zebrany
            doubleShotSpeed = true;  // Aktywuj efekt podwójnej prêdkoœci strza³ów
            powerUpTimer.restart();  // Zrestartuj zegar dzia³ania power-upa
        }

        if (doubleShotSpeed && powerUpTimer.getElapsedTime().asSeconds() > powerUpDuration) {
            doubleShotSpeed = false;  // Wy³¹cz efekt power-upa
        }

        // Handle displaying the wave message
        if (showWaveMessage) {
            if (waveMessageClock.getElapsedTime().asSeconds() >= 2.0f) { // Show message for 2 seconds
                showWaveMessage = false;
            }
        }

        float currentTime = enemyShootClock.getElapsedTime().asSeconds();//zegar do strzelania przeciwnika

        for (auto& star : stars) {
            star.move(STAR_SPEED);
        }


        //ruch gracza
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            if (shootClock.getElapsedTime().asSeconds() >= shootDelay) {
                bullets.emplace_back(
                    player.sprite.getPosition().x + Player_Width / 2 - Bullet_Width / 2,
                    player.sprite.getPosition().y
                );
                shootClock.restart();
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            player.move(-7.0f);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            player.move(7.0f);
        }

        for (auto& bullet : bullets) {
            bullet.update();
        }

        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](Bullet& b) { return !b.active; }), bullets.end());

        for (auto& bullet : bullets) {
            for (auto& enemy : enemies) {
                if (enemy.active && bullet.active && bullet.shape.getGlobalBounds().intersects(enemy.sprite.getGlobalBounds())) {
                    bullet.active = false;
                    enemy.active = false;
                    explosions.emplace_back(enemy.sprite.getPosition().x + Enemy_Width / 2, enemy.sprite.getPosition().y + Enemy_Height / 2);
                }
            }
        }

        moveEnemies(enemies, enemySpeedX, moveDown);

        // Logika koñca gry, gdy przeciwnik dotyka gracza lub dolnej krawêdzi ekranu
        for (auto& enemy : enemies) {
            if (enemy.active) {
                // Sprawdzamy, czy przeciwnik dotkn¹³ gracza
                if (enemy.sprite.getGlobalBounds().intersects(player.sprite.getGlobalBounds())) {
                    gameOver = true;  // Koniec gry
                }

                // Sprawdzamy, czy przeciwnik dotar³ do dolnej krawêdzi ekranu
                if (enemy.sprite.getPosition().y + Enemy_Height >= Window_Height) {
                    gameOver = true;  // Koniec gry
                }
            }
        }

        //strzelanie przeciwnika
        if (enemyShootClock.getElapsedTime().asSeconds() >= enemyShootDelay) {
            std::vector<Enemy*> activeEnemies;

            for (auto& enemy : enemies) {
                if (enemy.active) {
                    activeEnemies.push_back(&enemy);
                }
            }

            for (auto& enemyBullet : enemyBullets) {
                enemyBullet.moveDown();
            }

            if (!activeEnemies.empty()) {
                int randomIndex = std::rand() % activeEnemies.size();
                Enemy* chosenEnemy = activeEnemies[randomIndex];

                float currentTime = enemyShootClock.getElapsedTime().asSeconds();
                if (chosenEnemy->canShoot(currentTime)) {
                    float bulletX = chosenEnemy->sprite.getPosition().x + Enemy_Width / 2 - 2.5f;
                    float bulletY = chosenEnemy->sprite.getPosition().y + Enemy_Height;
                    enemyBullets.emplace_back(bulletX, bulletY);

                    chosenEnemy->lastShotTime = currentTime;
                    enemyShootClock.restart();
                }
            }
        }

        //Logika kolizji pociskow z graczem
        for (auto& enemyBullet : enemyBullets) {
            if (enemyBullet.shape.getGlobalBounds().intersects(player.sprite.getGlobalBounds())) {
                enemyBullet.shape.setPosition(-10, -10);  // Usuwamy pocisk
                player.playerLives--;  // Zmniejszamy liczbê ¿yæ

                // Sprawdzamy, czy gracz nie straci³ wszystkich ¿yæ
                if (player.playerLives <= 0) {
                    gameOver = true;  // Ustawiamy flagê gameOver
                }
            }
        }

        //usuwanie pociskow przeciwnika
        enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(),
            [](const EnemyBullet& b) { return b.shape.getPosition().y > Window_Height; }),
            enemyBullets.end());

        //tworzenie eksplozji
        for (auto& explosion : explosions) {
            explosion.update(deltaTime);
        }
        explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
            [](Explosion& e) { return !e.active; }), explosions.end());

        window.clear(sf::Color::Black);

        //rysowanie elementów gry



        for (const auto& star : stars) {
            window.draw(star.shape);
        }

        for (auto& bullet : bullets) {
            window.draw(bullet.shape);
        }

        window.draw(player.sprite);

        for (auto& enemy : enemies) {
            if (enemy.active) {
                window.draw(enemy.sprite);
            }
        }

        for (auto& explosion : explosions) {
            if (explosion.active) {
                window.draw(explosion.shape);
            }
        }

        for (const auto& enemyBullet : enemyBullets) {
            window.draw(enemyBullet.shape);
        }

        if (powerUp.active) {
            window.draw(powerUp.sprite);
        }

        // Draw wave message if active
        if (showWaveMessage) {
            window.draw(waveText);
        }

        drawHealthBar(window, player.playerLives);

        window.display();
    }

    return 0;

}
