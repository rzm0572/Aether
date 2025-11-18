#include <string>
#include "shader.h"
#include <glm/glm.hpp>

/**
 * @see 天空盒代码主要来源于：一步步学OpenGL(25) -《Skybox天空盒子》 - Kam92.J的文章 - 知乎 https://zhuanlan.zhihu.com/p/150570683
 * @see 将图像接口替换为 stb_image.h，参考 stb_image图像解码库 - 李钢蛋的文章 - 知乎 https://zhuanlan.zhihu.com/p/466294684
*/


/**
 * @brief 天空盒类，可以实现根据图片位置构造天空盒，调节视角、相机等，可以绘制天空盒
 * 
 * 具体的实现思路是：
 *      1. 类 CubemapTexture 负责加载立方体贴图纹理，并绑定到某个纹理单元插槽，如GL_TEXTURE0，代码中的体现是 cubemap->Bind(GL_TEXTURE0);
 *      2. 类 SkyboxMesh 负责绘制天空盒的立方体网格，由8个顶点组成，每个顶点有3个坐标值(x,y,z)，绑定到顶点缓冲区，并绑定到VAO。
 *      3. 类 Skybox 绘制天空盒，shader.setUniform("gCubemapTexture", 0); 绑定立方体贴图纹理到uniform变量 gCubemapTexture，与 GL_TEXTURE0 插槽对应
 * 
 * @warning 你需要构造天空盒，修改相机、视角等必要参数之后才能合理的绘制天空盒
 * @note 你可以在循环外面定义这个类，然后在渲染循环中实例化并调用它的Render方法，这个类仅暴露了天空盒图片和相机的接口
 * @warning 你需要在每一帧，即每次循环前清除深度  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 
 * @example 示例：Skybox skybox(getAssetPath("skybox/right.jpg"), getAssetPath("skybox/left.jpg"), getAssetPath("skybox/top.jpg"), getAssetPath("skybox/bottom.jpg"), getAssetPath("skybox/front.jpg"), getAssetPath("skybox/back.jpg"), &shaderManager);
 *              skybox.changeCameraPos(glm::vec3(0.0f, 0.0f, 3.0f)); // 设置相机位置
 *              skybox.changeProjection(glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f)); // 设置投影矩阵
 *              skybox.changeView(glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f))); // 设置视角矩阵
 *              skybox.Render(); // 绘制天空盒
*/
class Skybox
{
public:
    /**
     * @brief 构造一个天空盒对象
     *
     * @param posX 立方体贴图+X面(右)的纹理文件路径
     * @param negX 立方体贴图-X面(左)的纹理文件路径
     * @param posY 立方体贴图+Y面(上)的纹理文件路径
     * @param negY 立方体贴图-Y面(下)的纹理文件路径
     * @param posZ 立方体贴图+Z面(前)的纹理文件路径
     * @param negZ 立方体贴图-Z面(后)的纹理文件路径
     * @param global_shaderManager 全局着色器管理器指针，用于注册天空盒着色器
     *
     *
     * @throws std::runtime_error 如果加载立方体贴图纹理失败
     *
     * @note 构造函数会初始化天空盒的立方体贴图、网格和着色器，
     *       并设置默认相机位置为(0,0,3)
     */
    Skybox(const std::string &posX, const std::string &negX,
        const std::string &posY, const std::string &negY,
        const std::string &posZ, const std::string &negZ,
        GL::ShaderManager *global_shaderManager);

    ~Skybox();

    /**
     * @brief 渲染天空盒
     *
     * 该方法负责渲染天空盒，执行以下操作：
     * 1. 保存当前OpenGL的剔除模式和深度测试状态
     * 2. 设置天空盒特有的渲染状态（正面剔除和LEQUAL深度测试）
     * 3. 使用天空盒着色器并设置必要的uniform变量
     * 4. 绑定立方体贴图纹理
     * 5. 渲染天空盒网格
     * 6. 恢复之前保存的OpenGL状态
     *
     * @note 该方法会修改OpenGL状态，调用后会自动恢复原始状态
     * @note 视图矩阵会移除平移分量，确保天空盒始终围绕相机
     *
     */
    void Render();

    /**
     * @brief 设置相机位置
     *
     * @param pos 新的相机位置坐标(glm::vec3类型)
     */

    void changeCameraPos(glm::vec3 pos)
    {
        cameraPos = pos;
    }
    /**
     * @brief 更新投影矩阵
     * 
     * @param proj 新的投影矩阵，将替换当前投影矩阵
     */
    void changeProjection(glm::mat4 proj){
        projection=proj;
    }
    /**
     * @brief 更新视图矩阵
     *
     * @param view_ 新的视图矩阵，用于替换当前视图矩阵
     */
    void changeView(glm::mat4 view_){
        view=view_;
    }

private: 
    /**
     * @brief CubemapTexture 天空盒纹理类
     * 通过文件路径导入六张图片形成一个盒子
     * 将盒子纹理绑定到某个纹理单元插槽，如GL_TEXTURE0
     * 
     * 

    * @example 示例：首先以相对于 asset 目录的六张正方形特定天空盒图片相对路径为参数的 CubemapTexture 构造函数的使用示例：
    * CubemapTexture skyboxTexture("skybox/right.jpg", "skybox/left.jpg", "skybox/top.jpg", "skybox/bottom.jpg", "skybox/front.jpg", "skybox/back.jpg");
    * cubemap->load()
    * cubemap->Bind(GL_TEXTURE0);
    * 
    * @see 目前当且仅当在 Skybox 中用于导入天空盒纹理时，才会使用到此类，用于将读取的天空盒图片绑定到插槽 GL_TEXTURE0。

    */
    class CubemapTexture;   // 天空盒纹理类，用于加载天空盒纹理
    /**
     * @brief 天空盒网格类
     * 简单的立方体网格（用于天空盒）
     * 
     * 用于渲染天空盒的立方体网格，由8个顶点组成，每个顶点有3个坐标值(x,y,z)。
     * @note 构造函数会自动完成顶点的分配，
     * 
     * @see 目前当且仅当在 Skybox 中用于渲染天空盒网格时，才会使用到此类。

    */
    class SkyboxMesh;       // 天空盒网格类，用于绘制天空盒
    CubemapTexture* cubemap;// 天空盒纹理
    SkyboxMesh* mesh;// 天空盒网格
    GL::ShaderManager* shaderManager;// 天空盒着色器管理器，构造函数中注册shader，需要shader时取出即可
    glm::vec3 cameraPos;// 相机位置
    glm::mat4 projection;// 投影矩阵
    glm::mat4 view;// 视角矩阵
};