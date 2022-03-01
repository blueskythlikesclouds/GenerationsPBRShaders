namespace GensShaderTool.Shaders;

public static class ShaderHandle<T> where T : IShader, new()
{
    private static T sShader;

    public static T Reference => sShader ??= new T();
}