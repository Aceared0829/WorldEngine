#include "TestFramework.as"

enum Phase
{
    Init,
    Render,

    Delete,
    CheckDelete,

    Done
}

class ScriptObject : WAngelScriptTestClass
{
    WTime RenderDelay = WTime::Seconds(0.2);

    private Phase m_Phase = Phase::Init;
    private WGameObjectHandle m_hChild1;
    private WGameObjectHandle m_hChild2;
    private WComponentHandle m_hFog;
    private WComponentHandle m_hLight;
    private WComponentHandle m_hBlackboard;

    private WTime m_tStart;

    ScriptObject()
    {
        super("GameObjectTest");
    }

    bool ExecuteTests()
    {
        if (m_Phase == Phase::Init)
        {
            W_TEST_BOOL(m_hChild1.IsInvalidated());
            W_TEST_BOOL(m_hChild2.IsInvalidated());

            WGameObject@ obj1 = GetOwner().FindChildByName("Child1");
            m_hChild1 = obj1.GetHandle();

            WGameObject@ obj2 = GetOwner().FindChildByPath("Child2");
            m_hChild2 = obj2.GetHandle();

            W_TEST_BOOL(!m_hChild1.IsInvalidated());
            W_TEST_BOOL(!m_hChild2.IsInvalidated());
            W_TEST_BOOL(m_hChild1 == m_hChild1);
            W_TEST_BOOL(m_hChild2 == m_hChild2);
            W_TEST_BOOL(m_hChild1 != m_hChild2);

            W_TEST_BOOL(obj1.GetParent().HasName("GameObject Test"));
            W_TEST_BOOL(@obj1.GetWorld() == @GetWorld());

            W_TEST_BOOL(!obj1.HasTag("Dummy"));
            W_TEST_BOOL(obj2.HasTag("Dummy"));

            obj1.MakeDynamic();

            WFogComponent@ fog;
            W_TEST_BOOL(obj1.TryGetComponentOfBaseType(@fog));

            W_TEST_BOOL(m_hFog.IsInvalidated());
            m_hFog = fog.GetHandle();
            W_TEST_BOOL(!m_hFog.IsInvalidated());

            fog.Color = WColor::OrangeRed;
            fog.Density = 5;

            WPointLightComponent@ light;
            W_TEST_BOOL(obj2.TryGetComponentOfBaseType(@light));
            m_hLight = light.GetHandle();
            W_TEST_BOOL(!light.Active);
            light.Active = true;

            m_tStart = WTime::Now();
            m_Phase = Phase::Render;

            WGameObjectDesc gd;
            gd.m_LocalPosition.Set(0, 0, 10);
            
            WGameObject@ spawnObj;
            GetWorld().CreateObject(gd, @spawnObj);

            WSpawnComponent@ spawnCmp;
            spawnObj.CreateComponent(@spawnCmp);

            spawnCmp.Prefab = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }";
            spawnCmp.Deviation = WAngle::MakeFromDegree(30);
            spawnCmp.TriggerManualSpawn(true, WVec3::MakeZero());

            WAlwaysVisibleComponent@ vis;
            spawnObj.CreateComponent(@vis); // needed for the text to show up

            WDebugTextComponent@ text;
            spawnObj.CreateComponent(@text);
            text.MaxDistance = 1000;
            text.Color = WColor::LightGreen;
            text.Text = "Hello World! ({})";
            text.Value0 = 42;

            WGameObject@ boxObj;
            gd.m_LocalPosition.z = 7;
            GetWorld().CreateObject(gd, @boxObj);

            WMeshComponent@ mesh;
            boxObj.CreateComponent(@mesh);
            mesh.Mesh = "{ e9e6b167-c18c-4260-9b0e-4cf4503c1463 }";
            mesh.Color = WColor::BlueViolet * 10;
            mesh.CustomData = WVec4(1, 2, 3, 4);

            WGameObject@ skybox;
            W_TEST_BOOL(GetWorld().TryGetObjectWithGlobalKey("Skybox", skybox));

            WGameObject@ skybox2 = GetWorld().SearchForObject("G:Skybox");
            W_TEST_BOOL(@skybox == @skybox2);

            WSkyBoxComponent@ skyboxComp;
            W_TEST_BOOL(skybox.TryGetComponentOfBaseType(@skyboxComp));

            skyboxComp.CubeMap = "{ da307f05-9ef4-4a09-81d4-b86f5be99412 }";

            WGameObject@ bbObj;
            gd.m_LocalPosition.z = 7;
            gd.m_LocalPosition.y = 5;
            GetWorld().CreateObject(gd, @bbObj);            

            WLocalBlackboardComponent@ bbComp;
            bbObj.CreateComponent(@bbComp);
            m_hBlackboard = bbComp.GetHandle();

            bbComp.SetEntryValue("a", 1);
            bbComp.SetEntryValue("b", 2.0f);
            bbComp.SetEntryValue("c", WColor::RebeccaPurple);
            bbComp.SetEntryValue("d", WVec3(1, 2, 3));
            bbComp.SetEntryValue("e", WHashedString("HS"));
            bbComp.SetEntryValue("f", WColorGammaUB(50, 100, 150, 250));
            bbComp.SetEntryValue("g", m_hChild1);
            bbComp.SetEntryValue("h", m_hFog);
        }
        else if (m_Phase == Phase::Render)
        {
            WFogComponent@ fog;
            W_TEST_BOOL(GetWorld().TryGetComponent(m_hFog, @fog));

            const float fLerp = WMath::Min(1.0f, ((WTime::Now() - m_tStart) / RenderDelay).AsFloatInSeconds());

            fog.Color = WMath::Lerp(WColor::OrangeRed, WColor::CornflowerBlue, fLerp);

            WWorld@ fogWorld = fog.GetWorld();
            WGameObject@ fogObj = fog.GetOwner();
            fog.GetOwner().SetGlobalPosition(WMath::Lerp(WVec3(0, 0, 5), WVec3(0, 0, 10), fLerp));

            if (WTime::Now() - m_tStart >= RenderDelay)
            {
                WPointLightComponent@ light;
                W_TEST_BOOL(GetWorld().TryGetComponent(m_hLight, @light));
                light.Active = false;

                fogObj.SetActiveFlag(false);
    
                m_Phase = Phase::Delete;
            }
        }
        else if (m_Phase == Phase::Delete)
        {
            GetWorld().DeleteObjectDelayed(m_hChild1);

            WMsgDeleteGameObject msg;
            GetWorld().PostMessage(m_hChild2, msg, WTime::MakeZero(), WObjectMsgQueueType::NextFrame);

            {
                WLocalBlackboardComponent@ bbComp;
                W_TEST_BOOL(GetWorld().TryGetComponent(m_hBlackboard, @bbComp));

                W_TEST_INT(bbComp.GetEntryValue_asInt32("a"), 1);
                W_TEST_FLOAT(bbComp.GetEntryValue_asFloat("b"), 2);
                W_TEST_COLOR(bbComp.GetEntryValue_asColor("c"), WColor::RebeccaPurple);
                W_TEST_VEC3(bbComp.GetEntryValue_asVec3("d"), WVec3(1, 2, 3));
                W_TEST_STRING(bbComp.GetEntryValue_asString("e"), "HS");
                W_TEST_COLOR(bbComp.GetEntryValue_asColor("f"), WColorGammaUB(50, 100, 150, 250));
                W_TEST_BOOL(bbComp.GetEntryValue_asGameObjectHandle("g") == m_hChild1);
                W_TEST_BOOL(bbComp.GetEntryValue_asComponentHandle("h") == m_hFog);
            }
            
            m_Phase = Phase::CheckDelete;
        }
        else if (m_Phase == Phase::CheckDelete)
        {
            WGameObject@ obj;

            W_TEST_BOOL(!GetWorld().TryGetObject(m_hChild1, obj));
            W_TEST_BOOL(!GetWorld().TryGetObject(m_hChild2, obj));
            
            m_Phase = Phase::Done;
        }

        return m_Phase != Phase::Done;
    }
}