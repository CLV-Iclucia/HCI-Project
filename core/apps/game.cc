#include <array>
#include <cstring>
#include <format>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ogl-render/ogl-ctx.h>
#include <ogl-render/shader-prog.h>
#include <ogl-render/camera.h>
#include <iostream>
#include <iostream>
#include <vector>
#include <random>
#include <tbb/tbb.h>
#define ERROR(msg) do {std::cout << std::format("Error: {}", msg) << std::endl; exit(1);} while(0)

using namespace opengl;
bool initGLFW(GLFWwindow*&window) {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return false;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window = glfwCreateWindow(1280,
                            720,
                            "SURFACE-DEMO",
                            nullptr,
                            nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  return true;
}

struct PhongMaterial {
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  float shininess = 32.0;
  PhongMaterial(glm::vec3 ambient_ = glm::vec3(1.f, 0.5f, 0.31f),
                glm::vec3 diffuse_ = glm::vec3(1.f, 0.5f, 0.31f),
                glm::vec3 specular_ = glm::vec3(1.f, 1.f, 1.f))
    : ambient(std::move(ambient_)), diffuse(std::move(diffuse_)),
      specular(std::move(specular_)) {
  }
  void setUniformInstance(ShaderProg&shader, const std::string&instanceName) {
    shader.setVec3f(instanceName + std::string(".ambient"), ambient);
    shader.setVec3f(instanceName + std::string(".diffuse"), diffuse);
    shader.setVec3f(instanceName + std::string(".specular"), specular);
    shader.setFloat(instanceName + std::string(".shininess"), shininess);
  }
} phong;

struct Light {
  glm::vec3 position;
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  Light(glm::vec3 ambient_ = glm::vec3(0.1f, 0.1f, 0.1f),
        glm::vec3 diffuse_ = glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3 specular_ = glm::vec3(1.0f, 1.0f, 1.0f))
    : ambient(std::move(ambient_)), diffuse(std::move(diffuse_)),
      specular(std::move(specular_)) {
  }
  void setUniformInstance(ShaderProg&shader, const std::string&instanceName) {
    shader.setVec3f(instanceName + std::string(".position"), position);
    shader.setVec3f(instanceName + std::string(".ambient"), ambient);
    shader.setVec3f(instanceName + std::string(".diffuse"), diffuse);
    shader.setVec3f(instanceName + std::string(".specular"), specular);
  }
};

FpsCamera camera;
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  camera.processMouseScroll(yoffset);
}

void processWindowInput(GLFWwindow* window, FpsCamera&camera) {
  static float lastFrame = 0.f;
  float currentFrame = glfwGetTime();
  float deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;
  camera.processKeyBoard(window, deltaTime);
}


struct Range {
  int begin;
  int end;
};

class RandomGenerator {
  public:
    RandomGenerator() : m_gen(std::random_device{}()) {
    }
    int generate(int min, int max) {
      int rnd = distrib(m_gen);
      return rnd % (max - min) + min;
    }
    int generate(Range range) {
      int rnd = distrib(m_gen);
      return rnd % (range.end - range.begin) + range.begin;
    }

  private:
    std::uniform_int_distribution<> distrib;
    std::mt19937 m_gen;
};

struct Point {
  Point() = default;
  Point(int x, int y) : x(x), y(y) {}
  int x{};
  int y{};
};

const std::array<Point, 4> coordChanges{
  Point(0, -1),
  Point(0, 1),
  Point(-1, 0),
  Point(1, 0),
};

enum class Action : uint8_t { Up, Down, Left, Right, Switch };
enum class TileState : uint8_t { Empty = 0, Gray = 1, Black = 2, White = 3 };

void clearScreen() {
#ifdef WINDOWS
  std::system("cls");
#else
  // Assume POSIX
  std::system ("clear");
#endif
}

class Map : NonCopyable {
  public:
    [[nodiscard]] TileState tile(int x, int y) const {
      assert(x >= 0 && x < width && y >= 0 && y < height);
      return tiles[y * width + x];
    }
    TileState& tile(int x, int y) {
      assert(x >= 0 && x < width && y >= 0 && y < height);
      return tiles[y * width + x];
    }
    [[nodiscard]] TileState tile(Point p) const {
      assert(p.x >= 0 && p.x < width && p.y >= 0 && p.y < height);
      return tiles[p.y * width + p.x];
    }
    TileState& tile(Point p) {
      assert(p.x >= 0 && p.x < width && p.y >= 0 && p.y < height);
      return tiles[p.y * width + p.x];
    }
    Map(int numPaths, int minPathLen, int maxPathLen) {
      std::vector<Action> actions;
      std::vector<int> actionLengths(numPaths);
      for (int i = 0; i < numPaths; i++) {
        int pathLen = randGen.generate(minPathLen, maxPathLen);
        for (int j = 0; j < pathLen; j++) {
          Action action = static_cast<Action>(randGen.generate(0, 4));
          actions.push_back(action);
          actionLengths[i] = pathLen;
        }
      }
      start = computeMapInfo(actions, actionLengths, width, height);
      tiles.resize(width * height);
      int curPathStart = 0;
      TileState curColor = TileState::Black;
      for (auto len : actionLengths) {
        Point pos{start};
        for (int i = curPathStart; i < curPathStart + len; ++i) {
          Action action = actions[i];
          if (tile(pos) != TileState::Empty || action == Action::Switch)
            tile(pos) = TileState::Gray;
          else
            tile(pos) = curColor;
          if (action == Action::Switch) {
            if (curColor == TileState::Black)
              curColor = TileState::White;
            else
              curColor = TileState::Black;
            continue;
          }
          pos = {
            pos.x + coordChanges[static_cast<int>(action)].x,
            pos.y + coordChanges[static_cast<int>(action)].y
          };
        }
        curPathStart += len;
        exits.push_back(pos);
      }
    }

    Map(Map&&other) noexcept
      : tiles(std::move(other.tiles)), start(other.start),
        width(other.width), height(other.height) {
      other.width = 0;
      other.height = 0;
    }

    Map& operator=(Map&&other) noexcept {
      if (this != &other) {
        tiles = std::move(other.tiles);
        start = other.start;
        width = other.width;
        height = other.height;
        other.width = 0;
        other.height = 0;
      }
      return *this;
    }

    void getMapInfo() const {
      std::cout << std::format("Map width: {}, height: {}", width, height) << std::endl;
      std::cout << std::format("Start point: ({}, {})", start.x, start.y) << std::endl;
    }
    [[nodiscard]] Point getStart() const {
      return start;
    }
    [[nodiscard]] int getWidth() const {
      return width;
    }

    [[nodiscard]] int getHeight() const {
      return height;
    }

    [[nodiscard]] const std::vector<Point>& getExits() const {
      return exits;
    }
  private:
    std::vector<TileState> tiles;
    std::vector<Point> exits;
    Point start;
    int width, height;
    static Point computeMapInfo(const std::vector<Action>&actions,
                         const std::vector<int>&actionLengths,
                         int&width,
                         int&height) {
      Point currentPos = {0, 0};
      int minX = 0, maxX = 0, minY = 0, maxY = 0;
      int curPathStart = 0;
      for (auto len : actionLengths) {
        Point pos{0, 0};
        for (int i = curPathStart; i < curPathStart + len; ++i) {
          Action action = actions[i];
          if (action == Action::Switch)
            continue;
          pos = {
            pos.x + coordChanges[static_cast<int>(action)].x,
            pos.y + coordChanges[static_cast<int>(action)].y
          };
          minX = std::min(minX, pos.x);
          maxX = std::max(maxX, pos.x);
          minY = std::min(minY, pos.y);
          maxY = std::max(maxY, pos.y);
        }
        curPathStart += len;
      }
      width = maxX - minX + 1;
      height = maxY - minY + 1;
      return {-minX, -minY};
    }
    RandomGenerator randGen;
};

enum class GameEnd {
  Finished,
  Failed,
  Running,
};

struct GameState {
  explicit GameState(Point p) : pos(p), time(0.f) {}
  void move(Action action) {
    if (action == Action::Up)
      pos.x--;
    else if (action == Action::Down)
      pos.x++;
    else if (action == Action::Left)
      pos.y--;
    else if (action == Action::Right)
      pos.y++;
    else ERROR("unknown action");
  }
  void update(const Map& map) {
    if (pos.x < 0 || pos.x >= map.getWidth() || pos.y < 0 || pos.y >= map.getHeight())
      ending = GameEnd::Failed;
    if (color != TileState::Black && color != TileState::White)
      ERROR("invalid color");
    if (map.tile(pos) == TileState::Empty)
      ending = GameEnd::Failed;
    if (map.tile(pos) == TileState::Black && color == TileState::White)
      ending = GameEnd::Failed;
    if (map.tile(pos) == TileState::White && color == TileState::Black)
      ending = GameEnd::Failed;
    for (auto end : map.getExits()) {
      if (pos.x == end.x && pos.y == end.y) {
        ending = GameEnd::Finished;
        return ;
      }
    }
  }
  GameEnd ending{GameEnd::Running};
  Point pos;
  TileState color{TileState::Black};
  float time;
};

struct InputAdapter {
  virtual void inputAction() = 0;
  virtual ~InputAdapter() = default;
  tbb::concurrent_queue<Action> buffer;
};

class KeyboardAdapter final : public InputAdapter {
  public:
    KeyboardAdapter() {
      tg.run([this]() {inputAction();});
    }
    void inputAction() override {
      char c = static_cast<char>(getchar());
      while (running) {
        if (c == 'a' || c == 'A') {
          buffer.push(Action::Left);
        }
        if (c == 'w' || c == 'W')
          buffer.push(Action::Up);
        if (c == 's' || c == 'S')
          buffer.push(Action::Down);
        if (c == 'd' || c == 'D')
          buffer.push(Action::Right);
        if (c == 'x' || c == 'X')
          buffer.push(Action::Switch);
        c = static_cast<char>(getchar());
      }
    }
    ~KeyboardAdapter() noexcept override {
      running = false;
      tg.wait();
    }
  private:
    std::atomic_bool running{true};
    tbb::task_group tg;
};

struct Displayer : NonCopyable {
  virtual void display(const Map&map, const GameState&state) = 0;
  virtual ~Displayer() = default;
};

class AsciiArtDisplayer final : public Displayer {
  public:
    AsciiArtDisplayer() = default;
    void display(const Map&map, const GameState&state) override {
      clearScreen();
      std::cout << std::format("current position: ({}, {})\n{}", state.pos.x, state.pos.y, formString(map, state)) << std::endl;
    }
  private:
    static std::string formString(const Map&map, const GameState&state) {
      std::vector<char> buffer(map.getWidth() * (map.getHeight() + 1));
      for (int x = 0; x < map.getWidth(); ++x) {
        for (int y = 0; y < map.getHeight(); ++y) {
          if (map.tile(x, y) == TileState::Empty)
            buffer[x * (map.getHeight() + 1) + y] = ' ';
          else if (map.tile(x, y) == TileState::Gray)
            buffer[x * (map.getHeight() + 1) + y] = 'G';
          else if (map.tile(x, y) == TileState::Black)
            buffer[x * (map.getHeight() + 1) + y] = 'B';
          else if (map.tile(x, y) == TileState::White)
            buffer[x * (map.getHeight() + 1) + y] = 'W';
          else ERROR("unknown tile state");
          if (x == state.pos.x && y == state.pos.y)
            buffer[x * (map.getHeight() + 1) + y] = '#';
        }
        buffer[x * (map.getHeight() + 1) + map.getHeight()] = '\n';
      }
      for (auto end : map.getExits())
        buffer[end.x * (map.getHeight() + 1) + end.y] = 'E';
      return {buffer.begin(), buffer.end()};
    }
};

int main(int argc, char** argv) {
  auto map = std::make_unique<Map>(1, 30, 50);
  std::unique_ptr<Displayer> displayer = std::make_unique<AsciiArtDisplayer>();
  std::unique_ptr<InputAdapter> input = std::make_unique<KeyboardAdapter>();
  GameState state(map->getStart());
  while (state.ending == GameEnd::Running) {
    if (Action action; input->buffer.try_pop(action))
      state.update(*map);
    displayer->display(*map, state);
    sleep(1);
  }
  std::cout << "Game ended!" << std::endl;
}
