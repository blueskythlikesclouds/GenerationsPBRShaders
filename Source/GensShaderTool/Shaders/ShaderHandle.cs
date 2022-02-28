namespace GensShaderTool.Shaders;

public static class ShaderHandle<T> where T : Shader, new()
{
    private static T sShader;

    public static T Reference => sShader ??= new T();

    public static ShaderFeaturePair GetFeaturePair(params string[] features)
    {
        return Reference.GetFeaturePair(features);
    }
}