# Module Usage

## 资源管理

本项目中，游戏资源包括了着色器（Shader）、网格（Mesh）、纹理（Texture）、材质（Material）和模型（Model）。

其中，Shader、Texture 和 Material 资源通过管理类来存储和管理，Mesh 由 `Model` 类存储和管理，Model 目前未设计管理器，之后视需求再添加。

**注意**: `Shader`、`Mesh`、`Texture` 和 `Material` 等资源类禁用了拷贝构造和拷贝赋值，仅保留了移动构造和移动赋值，以避免对 OpenGL 资源的重复释放。


### 着色器管理

`Shader` 管理单个着色器，`ShaderManager` 类管理所有的 `Shader` 实例。

`Shader` 类提供了以下接口函数：

- `Shader()`: 构造函数，默认创建一个空的着色器。

- `Shader(const char* vertex_shader_path, const char* fragment_shader_path)`: 构造函数，根据传入的顶点着色器和片元着色器文件路径，加载并编译着色器。

- `useShader() const`: 激活当前 `Shader` 对象管理的着色器。

- `bool setUniform(const std::string& name, T x, Args... args) const`: 设置着色器中的 uniform 变量的值。

    采用了变长参数模板，可以传入任意数量和类型的参数。例如：

    ```c++
    shader.setUniform("hasTexture", true);
    shader.setUniform("color", 1.0f, 1.0f, 1.0f, 1.0f);
    shader.setUniform("color", glm::vec4(1.0f));
    shader.setUniform("model", glm::mat4(1.0f));
    ```

- `unsigned int getShaderID() const`: 获取当前 `Shader` 对象管理的着色器的 ID。


`ShaderManager` 类提供了以下接口函数：

- `void registerShader(std::string name, const char* vertex_shader_path, const char* fragment_shader_path)`: 以 `name` 为搜索键，注册一个新的着色器，并根据传入的顶点着色器和片元着色器文件路径，加载并编译着色器。

- `void registerShader(std::string name, std::string vertex_shader_path, std::string fragment_shader_path)`: 上一个注册函数的重载。

- `const Shader* getShader(ShaderProgramID id)`: 根据 OpenGL 的着色器 ID 获取对应的 `Shader` 实例的指针。

- `const Shader* getShader(const std::string& name)`: **常用**，根据注册时使用的名称获取对应的 `Shader` 实例的指针。

- `const Shader* useShader(const std::string& name)`: **常用**，激活并返回 `name` 对应的 `Shader` 实例的指针。


### 纹理管理

`Texture` 管理单个纹理，`TextureManager` 类管理所有的 `Texture` 实例。

`Texture` 类提供了以下接口函数：

- `Texture(GLenum type, const std::string& filepath)`: 构造函数，根据传入的纹理类型和文件路径，加载并创建纹理。

- `bool Load()`: 将纹理从磁盘文件中加载到内存中，返回是否加载成功。

- `void Bind(GLenum texture_unit) const`: 绑定纹理到指定纹理单元。

- `GLuint getID() const`: 获取当前 `Texture` 对象管理的纹理的 OpenGL ID。

**注意**：OpenGL 纹理资源的生命周期由 `Texture` 实例管理。


`TextureManager` 类提供了以下接口函数：

- `std::shared_ptr<const Texture> getTexture(const std::string& filepath, GLenum type = GL_TEXTURE_2D)`: **常用**，先在已加载到内存中的纹理缓存中根据 `filepath` 查找对应纹理，若找到则直接返回，若未找到则尝试从 `filepath` 加载纹理并缓存，返回 `shared_ptr` 指针。

- `std::shared_ptr<const Texture> getDefaultTexture() const`: 获取默认纹理，即一个 1x1 的白色纹理，会在纹理缺失时使用。（目前未实现）

- `void clearCache()`: 清空纹理缓存。


### 材质管理

`Material` 管理单个材质，`MaterialManager` 类管理所有的 `Material` 实例。

`Material` 类包括了以下成员变量：

```c++
// 着色器对象指针
const Shader* shader_;

// 纹理插槽
std::array<bool, size_t(TextureType::_COUNT)> has_texture_ = { false };
std::array<std::shared_ptr<const Texture>, size_t(TextureType::_COUNT)> textures_;

// 材质属性
glm::vec4 base_colors_;                     // 材质的基本颜色
float metallic_ { 0.0f };                   // 材质的金属度
float roughness_ { 0.5f };                  // 材质的粗糙度
float specular_ { 0.35f };                  // 材质的高光系数
glm::vec3 specularColor_ { 1.0f };          // 材质的高光颜色
```

**Future Warning**: 之后会尝试实现 PBR 材质，因此成员变量中的材质属性可能也会用纹理来表示，放到 `texture_` 数组中。

`Material` 类提供了以下接口函数：

- `Material(const Shader* shader)`: 构造函数，创建一个使用 `shader` 着色器的材质。

- `Material(const Shader* shader, const aiMaterial* material, const std::filesystem::path& directory)`: 构造函数，根据 `aiMaterial` 实例和文件目录，创建材质。

- `bool loadMaterial(const aiMaterial* material, const std::filesystem::path& directory)`: 根据 `aiMaterial` 实例中的信息，从 `directory` 目录中加载材质。

    - 注册材质时，采用的搜索键结构为 `directory/name`，其中 `directory` 为材质所在的文件夹路径，`name` 为 `aiMaterial` 实例中对应材质的名称。

- `void apply()`: 激活材质（激活纹理并设置着色器参数，不激活着色器，需要在调用前手动激活）

- getter 和 setter 函数：无需解释，参考源代码 `include/resource/material.h`


`MaterialManager` 类提供了以下接口函数：

- `std::shared_ptr<Material> loadMaterial(const Shader* shader, const aiMaterial* material, const std::filesystem::path& directory)`: **常用**，根据 `directory` 查找材质缓存，如果找到则直接返回，如果没找到则创建一个 `Material` 实例，并调用 `loadMaterial` 函数加载材质，缓存并返回。

- `std::shared_ptr<Material> getMaterial(const std::string& material_key)`: 根据材质的搜索键（`directory`）获取材质，返回 `shared_ptr` 指针，找不到则返回默认材质。


### 模型和网格管理

`Model` 管理单个模型，负责多项管理功能：

1. 管理 OpenGL 资源的生命周期，包括 VAO，VBO，EBO。

2. 存储一系列 `Mesh` 实例，并为各个 `Mesh` 维护 OpenGL 资源和材质。

3. 拥有一套默认材质，通过指向 `MaterialManager` 中存储的 `Material` 实例的指针实现。

4. 根据 `aiScene` 中的 `aiNode` 树，创建 `ModelNode` 树，表示模型的层次结构，其节点结构如下：

    ```c++
    struct ModelNode {
        std::string name;                   // 节点名称 
        glm::mat4 local_transform;          // 节点相对父节点的变换矩阵
        std::vector<unsigned int> meshes;   // 节点拥有的 mesh 在 Model 中的编号
        std::vector<ModelNode> children;    // 子节点列表
    };
    ```

5. 实现模型导入功能，包括模型使用的所有网格、材质和纹理。


具体地，对于一个网格，其存储结构如下：

- CPU 端：存储在 Mesh 类中

    ```c++
    std::string name_;                                    // mesh 名称

    std::vector<Vertex> vertices_;                        // 顶点数据
    std::vector<vIndex> indices_;                         // 索引数据

    size_t index_offset_ {0};                             // 索引在 EBO 中的偏移量
    unsigned int material_index_ {INVALID_MATERIAL};      // 使用的材质在 Model 中的索引
    ```

- GPU 端：存储在 VAO，VBO，EBO 中，由 Model 管理

    一个 Model 中的所有 Mesh 共享同一个 VAO、VBO、EBO，顶点数据和索引数据在 VBO 和 EBO 中按照 Mesh 编号连续存储：

    ```
    *------------------*----------------------*-----------------------*-----------*--------------------------*
    |  VBO (vertices)  |  Vertices of Mesh 0  |   Vertices of Mesh 1  |    ...    |  Vertices of Mesh n - 1  |
    *------------------*----------------------*-----------------------*-----------*--------------------------*

    *------------------*----------------------*-----------------------*-----------*--------------------------*
    |  EBO (indices)   |  Indices of Mesh 0   |   Indices of Mesh 1   |    ...    |  Indices of Mesh n - 1   |
    *------------------*----------------------*-----------------------*-----------*--------------------------*
    ```

    VBO 中的数据与 Mesh 上存储的顶点数据是相同的，但 EBO 中的数据与 Mesh 上存储的索引数据是不同的，顶点在放到 VBO 上时其索引会有一个偏移量 `vertex_offset`，因此我们需要将 EBO 中的索引数据加上 `vertex_offset` 才能让 GPU 获得正确的顶点索引。

    同时，索引本身在 EBO 上的位置也发生了偏移，其偏移量为 `index_offset`，为了在绘制网格时能找到正确的索引位置，我们需要在 Mesh 中存储偏移量 `index_offset_`。


`Model` 类提供了以下接口函数：

- `Model()`: 构造函数，申请一个顶点数组对象 VAO，并创建一个空模型。

- `Model(const std::string& filename)`: **常用**，构造函数，申请一个顶点数组对象 VAO，并根据 `filename` 文件中存储的模型信息加载模型。

- `loadModel(const std::string& filepath)`: **常用**，根据 `filepath` 文件中存储的模型信息加载模型。

调试接口：

- `const std::string toString() const`：打印模型中包含的资源的信息

- `outputModelTree()`: 打印 `ModelNode` 树的结构信息

- `void render(glm::mat4 model, glm::mat4 view, glm::mat4 projection, const Light& light) const`: 根据 MVP 矩阵和光照信息渲染模型，用于测试，勿必在成品中使用，请使用渲染器 `Renderer` 进行统一渲染。

继承接口：

- `void allocGPU(std::vector<Vertex>& vertices, std::vector<vIndex>& indices)`: 为当前模型申请 GPU 资源，并传入顶点和索引数据。

- `void releaseGPU()`: 释放当前模型的 GPU 资源。

- `ModelNode& getRootNode()`: 获取模型的根节点。

- `unsigned int insertMesh(Mesh&& mesh)`: 插入一个新的 `Mesh` 实例，并返回其在 `meshes_` 中的索引。

- `unsigned int insertMaterial(std::shared_ptr<Material> material)`: 插入一个新的 `Material` 指针，并返回其在 `default_materials_` 中的索引。


## 服务定位器

`ShaderManager`、`TextureManager`、`MaterialManager` 等资源管理类都是全局唯一实例的，但是其初始化和释放资源的顺序需要显式声明，因而我们采用了服务定位器 + 引擎的设计模式：

`ServiceLocator<Service>` 是一个类模板，其结构如下：

```c++
template<typename Service>
class ServiceLocator {
public:
    // Provide a service instance to global access
    static void provide(Service* service) {
        instance = service;
    }
    
    // Get the single instance of the service
    static Service* get() {
        return instance;
    }

private:
    inline static Service* instance = nullptr;
};
```

`GameEngine` 类是一个针对服务的管理类，其工作流程如下：

- 在游戏开始时创建一个 `GameEngine` 类的实例，并在 `GameEngine` 类的构造函数中，按顺序初始化各个服务并使用 `ServiceLocator::provide` 函数启用服务的全局访问。

- 在游戏逻辑中，通过 `ServiceLocator::get` 函数获取各个服务的实例

- 游戏结束时，`GameEngine` 类随函数体析构，在析构函数中按与初始化相反的顺序逐个释放各个服务的资源，并使用 `ServiceLocator::provide` 函数禁用服务的全局访问。

示例：

```c++
class GameEngine {
public:
    GameEngine(Config& config): config(config), shader_manager(), texture_manager(), material_manager()
    {
        // Initialize managers and services and register them in service locators
        shader_manager.init();
        ServiceLocator<ShaderManager>::provide(&shader_manager);

        texture_manager.init();
        ServiceLocator<TextureManager>::provide(&texture_manager);

        material_manager.init();
        ServiceLocator<MaterialManager>::provide(&material_manager);
    }

    ~GameEngine() {
        // Terminate services and clean up managers
        ServiceLocator<MaterialManager>::provide(nullptr);
        material_manager.clear();

        ServiceLocator<TextureManager>::provide(nullptr);
        texture_manager.clear();

        ServiceLocator<ShaderManager>::provide(nullptr);
        shader_manager.clear();
    }

private:
    Config& config;
    ShaderManager shader_manager;
    TextureManager texture_manager;
    MaterialManager material_manager;
};
```

在工作代码中，我们可以直接通过 `ServiceLocator::get` 函数获取各个服务的实例：

```c++
auto shader_manager = ServiceLocator<ShaderManager>::get();
if (texture_manager == nullptr) {
    cerr << "Shader manager not initialized!" << endl;
    assert(false);
}
Shader* shader = shader_manager->useShader("my_shader");
```



## 游戏对象和组件模式

游戏对象由 `GameObject` 类实现，其提供了以下接口函数：

- 工厂函数

    - `static GameObject* createFromModel<GameObjectDerived>(const Model& model)`: **最常用**，根据 `Model` 实例包含的 ModelNode 树的结构创建一个 GameObject 树，并额外进行以下两项操作：

        - 若 ModelNode 树的叶子节点为包含了多个 mesh，我们会为每个 mesh 创建一个 GameObject，再创建一个不可渲染的节点作为它们的父节点，对应原来的 ModelNode。
        - 创建一个 `GameObjectDerived` 类型的实例作为 GameObject 树的父节点，并返回其指针。

    - `static GameObject* createFromModelTree(const ModelNode& node, const Model& model, GameObject* parent = nullptr)`: 以 `node` 为根节点，根据模型创建 GameObject 树，将根节点的父节点设为 `parent`，返回 `node` 对应的 GameObject 指针。

    - `static GameObject* createFromMesh(int meshIdx, const Model& model, GameObject* parent = nullptr)`: 以 `model` 存储的编号为 `meshIdx` 的 mesh 创建单个 GameObject，将其父节点设为 `parent`，返回 GameObject 指针。

- 构造函数

    - `GameObject(TransformComponent transform = TransformComponent(), RenderComponent render = RenderComponent())`: 构造函数，可以传入自定义的 `TransformComponent` 和 `RenderComponent` 实例，UUID 自增。

    - `GameObject(UUID_t uuid, TransformComponent transform = TransformComponent(), RenderComponent render = RenderComponent())`: 构造函数，可以传入 UUID 值。除非你很确定传入的 UUID 值是正确的，否则不建议使用该构造函数。

- Setters and Getters

    - `UUID_t GetUUID() const`: 获取 GameObject 的 UUID。

    - `TransformComponent& getTransformComponent()`: 获取 GameObject 的 `TransformComponent` 实例引用。

    - `RenderComponent& getRenderComponent()`: 获取 GameObject 的 `RenderComponent` 实例引用。


### 组件

`GameObject` 由一系列组件（Component）组成，组件负责实现 GameObject 的各种功能。

在纯面向组件的游戏架构中，`GameObject` 类相当于一个组件的集合，使用一个类似 `std::map<component_type, component_ptr>` 的数据结构来存储组件。但我们的游戏不需要这么复杂，因此我们采用了将 `GameObject` 作为基类，派生出具体的游戏对象类（如 `Plane`, `Missile`）的方式。

`GameObject` 类作为基类，提供了两个通用的组件：`TransformComponent` 和 `RenderComponent`，分别用于存储 GameObject 的位置和渲染信息。

- `TransformComponent`：存储了所属的 GameObject 的父节点和子节点的信息，以及 GameObject 相对其父节点的位置、旋转和缩放信息。

    成员变量：

    ```c++
    glm::vec3 position_ {0.0f, 0.0f, 0.0f};            // 父节点坐标系下的位置
    glm::quat rotation_ {1.0f, 0.0f, 0.0f, 0.0f};      // 父节点坐标系下的旋转
    glm::vec3 scale_ {1.0f, 1.0f, 1.0f};               // 父节点坐标系下的缩放因子

    glm::mat4 local_cache_ {1.0f};                     // 相对父节点的变换矩阵
    glm::mat4 global_cache_ {1.0f};                    // 相对世界坐标系的变换矩阵

    bool local_dirty_ {true};
    bool global_dirty_ {true};

    GameObject* parent_ {nullptr};                     // 父节点
    std::vector<GameObject*> children_;                // 子节点
    ```

    我们可以通过以下接口函数来操作 `TransformComponent`：

    - `glm::mat4 getLocalModelMatrix()`: 获取 GameObject 相对于其父节点的变换矩阵。

    - `glm::mat4 getGlobalModelMatrix()`: 获取 GameObject 相对于世界坐标系的变换矩阵。

    - `void setlocalModelMatrix(const glm::mat4& local_model_matrix)`: 根据传入的变换矩阵，分解得到 position、rotation、scale，并更新 TransformComponent 中存储的相应值。

    - `void setParent(GameObject* parent, GameObject* owner)`: 将所属的 GameObject 的父节点设置为 `parent`，并更新旧的父节点和新的父节点的子节点列表。

    - 一系列 getter 函数：`getPosition()`, `getRotation()`, `getScale()`, `getChildren()`

    - 一些用于更新位置的函数，方便逻辑更新：

        - `void translate(glm::vec3 translation)`: 在父节点坐标系下进行 `translation` 描述的位移。

        - `void apply_velocity(glm::vec3 velocity, float dt)`: 在父节点坐标系下以 `velocity` 速度和 `dt` 时间步长更新位置。

        - `void rotate(glm::quat rotation)`: 在自身的局部坐标系下进行 `rotation` 描述的旋转。

        - `void rotate(glm::vec3 axis, float angle)`: 在自身的局部坐标系下沿着 `axis` 轴旋转 `angle` 角度。

        - `void apply_angluar_velocity(glm::vec3 angular_velocity, float dt)`: 在自身的局部坐标系下以 `angular_velocity` 角速度和 `dt` 时间步长更新旋转。
    
    **注意**: `TransformComponent` 采用存储变换分量、缓存变换矩阵的模式，在游戏的逻辑帧中，我们通常会对 GameObject 的位置、旋转方向等信息进行更新，更新后，其本身的 `local_dirty_` 标志和其子树内的所有节点的 `global_dirty_` 标志会被置为 `true`，表示此时的变换矩阵缓存是脏的。在渲染器或碰撞检测等模块调用 `getGlobalModelMatrix` 函数或 `getLocalModelMatrix` 函数时，如果 `global_dirty_` 或 `local_dirty_` 标志为 `true`，则会重新计算变换矩阵并更新缓存，再返回正确的结果。

- `RenderComponent`：存储了所属的 GameObject 的渲染信息，包括指向 Mesh 和 Material 的指针。

    `RenderComponent` 的成员变量都是 public 的，可以直接访问：

    ```c++
    bool renderable_;                         // 是否可渲染
    GLuint VAO_;                              // 顶点数据对象，创建时会设置成与包含 mesh 的 Model 的 VAO 一致
    const Mesh* mesh_;                        // 指向 Mesh 实例的指针（实际存储在 Model 中）
    std::shared_ptr<Material> material_;      // 指向 Material 实例的指针（实际存储在 MaterialManager 中）
    ```

    构造函数：

    - `RenderComponent()`

    - `RenderComponent(GLuint VAO, const Mesh* mesh, const std::shared_ptr<Material>& material)`


## 渲染器

渲染器由 `Renderer` 类实现，其维护了一个渲染队列。在每一个渲染帧中，我们可以向渲染队列中提交需要渲染的 GameObject，在帧结束时，渲染器会将队列中所有的 GameObject 渲染到屏幕上，并清空渲染队列。

我们可以通过以下接口函数使用渲染器：

> 你需要先进行阴影贴图渲染 pass，然后再渲染主场景。

- `void beginShadowPass(const Light& light)`：开启阴影贴图渲染 pass

- `void submit_recursive_renderShadow(GameObject* obj) `： 递归地将 GameObject 及其所有子孙节点进行阴影贴图渲染。

- `void endShadowPass() `：结束阴影贴图渲染 pass

> 这里是主场景的渲染

- `void submit(GameObject* obj)`: 将 GameObject 提交到渲染队列中。

- `void submit_recursive(GameObject* obj)`: 通过调用 `submit` 函数，递归地将 GameObject 及其所有子孙节点提交到渲染队列中。

    推荐使用。与 `GameObject` 类的 `createFromModel` 函数搭配使用十分方便，只需将需要渲染的 GameObject 树的根节点作为参数传入该函数，即可完成整个 GameObject 树的渲染。

- `void render(glm::mat4 view, glm::mat4 projection, const Light& light)`: 将队列中所有的 GameObject 渲染到屏幕上。

**注意**：GameObject 的提交顺序和渲染顺序不保证相同，为了避免切换着色器和 VAO 的开销，`submit` 函数会为每个 GameObject 生成一个 key，用于标识此 GameObject 使用的着色器和 VAO，`render` 函数会根据 key 对渲染队列进行排序，最后再进行渲染。

使用范例：

```c++
int main() {
    // 创建渲染器
    Renderer renderer;

    // 加载模型
    Model model1, model2;
    model1.loadModel("path/to/your/model1");
    model2.loadModel("path/to/your/model2");

    // 创建 GameObject
    GameObject* obj1 = GameObject::createFromModel(model1);
    GameObject* obj2 = GameObject::createFromModel(model2);

    // Gameloop
    while (...) {
        // GameObject 更新
        // ...
        // 提交光线更新光照信息，为阴影贴图提供 light_view_metrix 将世界坐标转换为光照坐标，并绑定阴影贴图纹理
        renderer.beginShadowPass(light);        
        // 渲染阴影贴图
        renderer.submit_recursive_renderShadow(plane);
        renderer.submit_recursive_renderShadow(terrain_obj);
        // 结束 shadow pass，解绑 shadow map 纹理
        renderer.endShadowPass();
        // 向渲染队列中提交 GameObject
        renderer.submit_recursive(obj1);
        renderer.submit_recursive(obj2);

        // 渲染
        renderer.render(view, projection, light);

        // ...
    }
}
```


## 输入状态获取

输入由 `Input` 类实现和处理。其接口函数包括：

- `void pollEvents()`: 获取输入事件，并更新输入状态。

- `bool getKeyPressed(InputKey key) const`: 检查 `key` 是否被按下。

- `bool getKeyPressedDown(InputKey key) const`: 检查 `key` 是否刚刚被按下，适用于每次按键只需要判定一次的情况。

- `glm::vec2 getMouseMovement() const`: 获取鼠标在屏幕空间中的位移。

- `void endUpdate()`: 结束输入状态的更新，该函数需要在游戏循环中所有逻辑更新结束后调用。


### 键盘输入

如果某个逻辑模块需要获取当前的键盘输入，例如检查某个键是否被按下，可以按照以下步骤编写代码：

1. 检查 `include/interaction/input.h` 文件中的枚举类型 `InputKey` 是否包含所需的键盘按键，以及 `Input::KeyTable` 数组是否包含从 GLFW 键码到 `InputKey` 的映射。

    - 如果没有，则需要在 `InputKey` 枚举类型中添加相应的键盘按键，并在 `Input::KeyTable` 数组中添加映射关系。

2. 在需要获取键盘输入的逻辑模块中，引入头文件，将 `Input` 实例传入（假设为 `input`），并调用 `getKeyPressed` 或 `getKeyPressedDown` 函数。

    ```c++
    // 检查 W 键是否被按下
    if (input.getKeyPressed(InputKey::W)) {
        // ...
    }

    // 检查 Q 键是否刚被按下
    if (input.getKeyPressedDown(InputKey::Q)) {
        // ...
    }
    ```

### 鼠标输入

如果某个逻辑模块需要获取当前的鼠标输入，只需要在模块中传入 `Input` 实例，并调用 `getMouseMovement` 函数即可。

```c++
// 获取鼠标在屏幕空间中的位移
glm::vec2 movement = input.getMouseMovement();
```


## 摄像机

`Camera` 类是所有摄像机类的抽象基类，其接口函数包括：

- `glm::mat4 getViewMatrix() const`: 获取摄像机的视图矩阵，**常用**。
- `glm::mat4 getFrontVec() const`: 获取摄像机的前方向向量。
- `glm::mat4 getRightVec() const`: 获取摄像机的右方向向量。
- `glm::mat4 getUpVec() const`: 获取摄像机的上方向向量。
- `glm::vec3 getPosition() const`: 获取摄像机的位置，**常用**。

后续考虑将 projection matrix 也作为摄像机的成员变量。

目前实现了两种摄像机：`FreeCamera` 和 `ThirdPersonCamera`，即自由视角摄像机和第三人称摄像机。

摄像机类的主要工作流程：

- 创建一个摄像机实例 `FreeCamera` 或 `ThirdPersonCamera`，并设置其初始位置、视角、移动速度和鼠标敏感度等属性

- 游戏循环内：

    - 在逻辑更新完成后，渲染更新开始前，通过 `update` 函数更新摄像机的位置和视角

    - 调用 `getViewMatrix` 函数获取摄像机的视图矩阵，并将其传入渲染器的 `render` 函数中渲染场景。


### 自由视角摄像机

`FreeCamera` 类实现了自由视角摄像机，其接口函数包括：

- 构造函数

    ```c++
    FreeCamera(
        glm::vec3 position,                                    // 初始位置
        glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f),      // 世界坐标系的上向量
        float pitch = -90.0f,                                  // 初始俯仰角
        float yaw = 0.0f,                                      // 初始偏航角
        float speed = 5.0f,                                    // 移动速度
        float sensitivity = 0.06f                              // 鼠标敏感度
    )
    ```

- `void update(const FreeCameraInputTranslator& input_translator, float delta_time)`: 根据鼠标和键盘输入更新摄像机的位置和视角。

    该函数的输入参数 `input_translator` 是一个 `FreeCameraInputTranslator` 类的实例，其提供了鼠标和键盘输入的转译。

    - 创建：`FreeCameraInputTranslator(const Input& input)`，将 `Input` 实例传入，用于获取鼠标和键盘输入。

    - 使用：传入 `FreeCamera` 的 `update` 函数即可。

使用示例：

```c++
FreeCamera camera(
    glm::vec3(0.0f, 0.0f, 3.0f),   // 初始位置
    glm::vec3(0.0f, 1.0f, 0.0f),   // 世界坐标系的上向量
    0.0f, 0.0f,                    // 初始俯仰角和偏航角
    5.0f, 0.06f                    // 移动速度和鼠标敏感度
);
FreeCameraInputTranslator translator(input);   // 转译层

// Gameloop
while (...) {
    input.pollEvents();                        // 获取输入事件
    // ...
    camera.update(translator, delta_time);     // 更新摄像机
    glm::mat4 view = camera.getViewMatrix();   // 获取摄像机的视图矩阵
    // ...
    renderer.render(view, projection, light);  // 渲染场景
}
```

### 第三人称摄像机

`ThirdPersonCamera` 类实现了第三人称摄像机，其接口函数包括：

- 构造函数

    ```c++
    ThirdPersonCamera(
        GameObject* target,                                           // 跟随目标对象
        float distance = 10.0f,                                       // 跟随距离
        float pitch = 0.0f,                                           // 初始俯仰角
        float yaw = 0.0f,                                             // 初始偏航角
        float smooth_factor = 5.0f,                                   // 平滑因子（用于线性插值，减少抖动）
        float sensitivity = 0.06f,                                    // 鼠标敏感度
        glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f),               // 注视位置在目标的局部坐标系内的偏移
        glm::quat base_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)   // 摄像机的 worldUp 相对物体局部坐标系的 y 轴的旋转
    );
    ```

- `void update(glm::vec2 mouse_offset, float dt)`: 根据鼠标输入更新摄像机的位置和视角。

`ThirdPersonCamera` 实现了对摄像机位置的线性插值，以保证跟随目标的运动状态改变时，摄像机不会发生剧烈抖动。

使用示例：

```c++
ThirdPersonCamera third_person_camera(plane, 16.0f, 0.0f, 90.0f, 10.0f, 0.06f, glm::vec3(0.5f, 0.0f, 0.0f));

// Gameloop
while (...) {
    input.pollEvents();                                          // 获取输入事件
    // ...
    third_person_camera.update(input.getMouseMovement(), dt);    // 更新摄像机
    glm::mat4 view = third_person_camera.getViewMatrix();        // 获取摄像机的视图矩阵
    // ...
    renderer.render(view, projection, light);                    // 渲染场景
}
```

## 性能监控

游戏的性能监控由 `Profiler` 类实现，其提供了时间测量和统计的功能。

- `static Profiler& instance()`: 获取 `Profiler` 类的全局唯一实例。

- `Timer& get_timer(const std::string& name)`: 获取 `name` 对应的 `Timer` 实例，若不存在则创建一个。

- `void report()`: 输出所有 `Timer` 实例的统计信息。


`Timer` 类实现了计时器功能：

- `Timer(const std::string& name = "")`: 构造函数，传入 `name` 作为计时器的名称。

- `void start_clock()`: 启动计时器。

- `void end_clock()`: 停止计时器。

计时器可以重复启动和停止，`Timer` 会统计经过的总时间和启停次数。

使用范例：

```c++
// Gameloop
while (...) {
    // 游戏逻辑更新
    Profiler::instance().get_timer("logic").start_clock();
    // do something
    Profiler::instance().get_timer("logic").end_clock();

    // 渲染更新
    Profiler::instance().get_timer("render").start_clock();
    // do something
    Profiler::instance().get_timer("render").end_clock();
}

Profiler::instance().report();
```

未来会加入 enable / disable 功能，用于在 release 版本中关闭一部分性能监控。
