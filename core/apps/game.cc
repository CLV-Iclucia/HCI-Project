#include <array>
#include <condition_variable>
#include <cstring>
#include <format>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ogl-render/ogl-ctx.h>
#include <ogl-render/shader-prog.h>
#include <iostream>
#include <iostream>
#include <vector>
#include <random>
#include <sys/stat.h>
#include <queue>
#define ERROR(msg) do {std::cout << std::format("Error: {}", msg) << std::endl; exit(1);} while(0)

using namespace opengl;

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
  Point(int x, int y) : x(x), y(y) {
  }
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
  std::system("clear");
#endif
}

class Map : NonCopyable {
  public:
    [[nodiscard]] TileState tile(int x, int y) const {
      assert(x >= 0 && x < width && y >= 0 && y < height);
      return tiles[x * height + y];
    }
    TileState& tile(int x, int y) {
      assert(x >= 0 && x < width && y >= 0 && y < height);
      return tiles[x * height + y];
    }
    [[nodiscard]] TileState tile(Point p) const {
      assert(p.x >= 0 && p.x < width && p.y >= 0 && p.y < height);
      return tiles[p.x * height + p.y];
    }
    TileState& tile(Point p) {
      assert(p.x >= 0 && p.x < width && p.y >= 0 && p.y < height);
      return tiles[p.x * height + p.y];
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
        tile(pos.x, pos.y) = curColor;
        curPathStart += len;
        exits.push_back(pos);
        assert(tile(pos.x, pos.y) != TileState::Empty);
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

    [[nodiscard]] bool isExit(int i, int j) const {
      for (const auto&ex : exits)
        if (ex.x == i && ex.y == j)
          return true;
      return false;
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

constexpr float kOperationInterval = 0.2f; // sec
constexpr float kMaxGameTime = 60.0f; // sec

enum class GameEnd {
  Finished,
  Failed,
  Running,
};

struct GameState {
  explicit GameState(Point p) : pos(p), time(glfwGetTime()), startTime(time) {
  }
  void move(const Map&map, Action action) {
    lastOperationPos = pos;
    lastOperationTime = glfwGetTime();
    if (action == Action::Up)
      pos.x--;
    else if (action == Action::Down)
      pos.x++;
    else if (action == Action::Left)
      pos.y--;
    else if (action == Action::Right)
      pos.y++;
    else if (action == Action::Switch) {
      if (color == TileState::Black)
        color = TileState::White;
      else
        color = TileState::Black;
    }
    else
      ERROR("unknown action");
  }
  void update(const Map&map) {
    time = glfwGetTime();
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
        return;
      }
    }
    if (time > startTime + kMaxGameTime) {
      ending = GameEnd::Finished;
      return;
    }
    if (time > lastOperationTime + kOperationInterval) {
      displayPos = glm::vec2(
        -1.f + static_cast<float>(pos.x) * 2.0 / map.getWidth(),
        -1.f + static_cast<float>(pos.y) * 2.0 / map.getHeight()
      );
    }
    else {
      float ratio = (time - lastOperationTime) / kOperationInterval;
      float lerp_x = (1.f - ratio) * static_cast<float>(lastOperationPos.x) + pos.x * ratio;
      float lerp_y = (1.f - ratio) * static_cast<float>(lastOperationPos.y) + pos.y * ratio;
      displayPos = glm::vec2(-1.f + lerp_x * 2.f / map.getWidth(), -1.f + lerp_y * 2.f / map.getHeight());
    }
  }
  GameEnd ending{GameEnd::Running};
  Point pos, lastOperationPos{};
  TileState color{TileState::Black};
  glm::vec2 displayPos{};
  float time{}, lastOperationTime{}, startTime{};
};

template <typename T> class ThreadSafeQueue {
  public:
  ThreadSafeQueue() = default;
  bool empty() {
    std::lock_guard<std::mutex> lk(mtx);
    return q.empty();
  }
  void push(T v) {
    std::lock_guard<std::mutex> lk(mtx);
    q.push(v);
  }
  void ImmPush(T v) { q.push(v); }
  bool TryPop(T &v) {
    std::lock_guard<std::mutex> lk(mtx);
    if (q.empty())
      return false;
    v = q.front();
    q.pop();
    return true;
  }
  void WaitPop(T &v) {
    std::unique_lock<std::mutex> lk(mtx);
    cond.wait(lk, [this]() -> bool { return !q.empty(); });
    v = q.front();
    q.pop();
    lk.unlock();
  }

  private:
  std::mutex mtx;
  std::condition_variable cond;
  std::queue<T> q;
};

struct InputAdapter {
  virtual void inputAction() = 0;
  virtual ~InputAdapter() = default;
  ThreadSafeQueue<Action> buffer;
};

bool initGLFW(GLFWwindow*&window) {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return false;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window = glfwCreateWindow(640, 720, "HCI-GAME", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  return true;
}

struct DrawBoard {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> colors;
  std::vector<uint> idx;
  void addSquare(float x, float y, float z, float width, float height, const glm::vec3&color) {
    int num_vertices = positions.size();
    positions.emplace_back(x, y, z);
    positions.emplace_back(x + width, y, z);
    positions.emplace_back(x + width, y + height, z);
    positions.emplace_back(x, y + height, z);
    for (int i = 0; i < 4; ++i)
      colors.push_back(color);
    idx.push_back(num_vertices);
    idx.push_back(num_vertices + 1);
    idx.push_back(num_vertices + 2);
    idx.push_back(num_vertices);
    idx.push_back(num_vertices + 2);
    idx.push_back(num_vertices + 3);
  }
};

class PythonSerialAdapter final : public InputAdapter {
  public:
    explicit PythonSerialAdapter(const std::string&pythonScript) : pipe(
      popen(std::format("python {}", pythonScript).c_str(), "r"),
      pclose) {
      if (pipe == nullptr)
        ERROR("failed to open python script");
      thread = std::make_unique<std::thread>([this]() { inputAction(); });
    }
    void inputAction() override {
      char c;
      fscanf(pipe.get(), "%c", &c);
      while(c != '#') {
        if (fscanf(pipe.get(), "%c", &c) == EOF)
          ERROR("unexpected end of pipe");
        printf("%c\n", c);
      }
      int v = filterInput();
      if (v == -1)
        ERROR("unexpected end of pipe");
      while (running) {
        switch (v) {
          case 0:
            buffer.push(Action::Left);
            break;
          case 1:
            buffer.push(Action::Up);
            break;
          case 2:
            buffer.push(Action::Down);
            break;
          case 3:
            buffer.push(Action::Right);
            break;
          case 4:
            buffer.push(Action::Switch);
            break;
          default:
            ERROR("unknown input");
        }
        v = filterInput();
      }
    }
    ~PythonSerialAdapter() noexcept override {
      running = false;
      if (thread->joinable())
        thread->join();
    }

  private:
    int filterInput() const {
      static int prev = 998;
      int v = 998;
      if (fscanf(pipe.get(), "%d", &v) == EOF) {
        std::cerr << "Warning: reaching end of file" << std::endl;
        return -1;
      }
      printf("%d\n", v);
      while (v == prev || v == 998) {
        prev = v;
        if (fscanf(pipe.get(), "%d", &v) == EOF)
          return -1;
        printf("%d\n", v);
      }
      return v;
    }
    std::atomic_bool running{true};
    std::unique_ptr<std::thread> thread;
    std::unique_ptr<FILE, decltype(&pclose)> pipe;
};

class OglDisplayer {
  public:
    explicit OglDisplayer(const Map&map) : width(map.getWidth()), height(map.getHeight()) {
      initGLFW(window);
      if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        exit(1);
      }
      shader = std::make_unique<ShaderProg>(std::format("{}/2d-default.vs", SHADER_DIR).c_str(),
                                            std::format("{}/2d-default.fs", SHADER_DIR).c_str());
      for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
          glm::vec3 squareColor;
          glm::vec3 squarePosition;
          squarePosition.x = -1.0f + static_cast<float>(i) / width * 2.0f; // Set x position based on column index
          squarePosition.y = -1.0f + static_cast<float>(j) / height * 2.0f; // Set y position based on row index
          squarePosition.z = 0.0f;
          if (map.tile(i, j) == TileState::Empty) continue;
          if (map.isExit(i, j)) {
            squareColor = glm::vec3(0.0f, 1.0f, 0.0f);
            squarePosition.z = -0.5f;
          }
          else if (map.tile(i, j) == TileState::Black)
            squareColor = glm::vec3(0.0f, 0.0f, 0.0f);
          else if (map.tile(i, j) == TileState::Gray)
            squareColor = glm::vec3(0.5f, 0.5f, 0.5f);
          else if (map.tile(i, j) == TileState::White)
            squareColor = glm::vec3(1.0f, 1.0f, 1.0f);
          board.addSquare(squarePosition.x,
                          squarePosition.y,
                          squarePosition.z,
                          2.0f / width,
                          2.0f / height,
                          squareColor);
        }
      }
      bgCtx = std::make_unique<OpenGLContext>();
      blockInfoOffset = board.positions.size();
      board.addSquare(0, 0, 2.0f / width, 2.0f / height, -0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
      shader->initAttributeHandles();
      shader->initUniformHandles();
      bgCtx->vao.bind();
      bgCtx->newAttribute("aPos", board.positions, 3, 3 * sizeof(float), GL_FLOAT);
      bgCtx->newAttribute("aColor", board.colors, 3, 3 * sizeof(float), GL_FLOAT);
      bgCtx->ebo.bind();
      bgCtx->ebo.passData(board.idx);
      shader->use();
    }
    bool shouldClose(const GameState&state) const {
      return glfwWindowShouldClose(window) || state.ending != GameEnd::Running;
    }
    void updateBlockData(const GameState&state) {
      auto&vbo = bgCtx->vbo[bgCtx->attribute("aPos")];
      vbo.bind();
      board.positions[blockInfoOffset] = glm::vec3(state.displayPos[0], state.displayPos[1], -0.5f);
      board.positions[blockInfoOffset + 1] = glm::vec3(state.displayPos[0] + 2.0f / width, state.displayPos[1], -0.5f);
      board.positions[blockInfoOffset + 2] = glm::vec3(state.displayPos[0] + 2.0f / width,
                                                       state.displayPos[1] + 2.0f / height,
                                                       -0.5f);
      board.positions[blockInfoOffset + 3] = glm::vec3(state.displayPos[0], state.displayPos[1] + 2.0f / height, -0.5f);
      vbo.updateData(board.positions.data() + blockInfoOffset, blockInfoOffset, 4);
      auto&color_vbo = bgCtx->vbo[bgCtx->attribute("aColor")];
      color_vbo.bind();
      glm::vec3 blockColor = glm::vec3(0.0f, 0.0f, 1.0f);
      if (state.color == TileState::Black)
        blockColor = glm::vec3(1.0f, 0.0f, 0.0f);
      board.colors[blockInfoOffset] = blockColor;
      board.colors[blockInfoOffset + 1] = blockColor;
      board.colors[blockInfoOffset + 2] = blockColor;
      board.colors[blockInfoOffset + 3] = blockColor;
      color_vbo.updateData(board.colors.data() + blockInfoOffset, blockInfoOffset, 4);
    }
    void display(const Map&map, const GameState&state) const {
      glfwPollEvents();
      int wnd_width, wnd_height;
      glfwGetFramebufferSize(window, &wnd_width, &wnd_height);
      glViewport(0, 0, wnd_width, wnd_height);
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      OpenGLContext::draw(GL_TRIANGLES, board.idx.size());
      glfwSwapBuffers(window);
    }
    ~OglDisplayer() {
      glfwDestroyWindow(window);
      glfwTerminate();
    }

  private:
    GLFWwindow* window{};
    DrawBoard board;
    int width, height;
    int blockInfoOffset{};
    std::unique_ptr<ShaderProg> shader{};
    std::unique_ptr<OpenGLContext> bgCtx;
};

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Usage: game [python script path]" << std::endl;
    return 0;
  }
  std::string pythonScipt = argv[1];
  auto map = std::make_unique<Map>(1, 30, 50);
  std::unique_ptr<OglDisplayer> displayer = std::make_unique<OglDisplayer>(*map);
  std::unique_ptr<PythonSerialAdapter> input = std::make_unique<PythonSerialAdapter>(pythonScipt);
  GameState state(map->getStart());
  state.update(*map);
  displayer->updateBlockData(state);
  while (!displayer->shouldClose(state)) {
    glfwPollEvents();
    if (Action action; input->buffer.TryPop(action))
      state.move(*map, action);
    state.update(*map);
    displayer->updateBlockData(state);
    displayer->display(*map, state);
  }
  std::cout << "Game ended!" << std::endl;
}
