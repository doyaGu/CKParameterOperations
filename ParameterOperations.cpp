#include "CKAll.h"

#include "ParameterOperationTypes.h"

#ifdef CK_LIB
    #define CKGetPluginInfoCount			CKGet_ParamOp_PluginInfoCount
    #define CKGetPluginInfo					CKGet_ParamOp_PluginInfo
    #define g_PluginInfo					g_ParamOp_PluginInfo
#else
    #define CKGetPluginInfoCount			CKGetPluginInfoCount
    #define CKGetPluginInfo					CKGetPluginInfo
    #define g_PluginInfo					g_PluginInfo
#endif

CKPluginInfo g_PluginInfo;
#define PARAMOP_GUID CKGUID(0x4c8f620e, 0x64521f0a)

const char *ParamOpName = "Parameter Operations";

void CKInitializeOperationTypes(CKContext *context)
{
    CKParameterManager *pm = context->GetParameterManager();
    pm->RegisterOperationType(CKOGUID_MODULO, "Modulo");
    pm->RegisterOperationType(CKOGUID_SQUARE_ROOT, "Square Root");
    pm->RegisterOperationType(CKOGUID_SINE, "Sine");
    pm->RegisterOperationType(CKOGUID_GET_CURVE_POINT, "Get Curve Point");
    pm->RegisterOperationType(CKOGUID_COSINE, "Cosine");
    pm->RegisterOperationType(CKOGUID_TANGENT, "Tangent");
    pm->RegisterOperationType(CKOGUID_ARC_TANGENT_2, "Arc Tangent 2");
    pm->RegisterOperationType(CKOGUID_ARC_SINE, "Arc Sine");
    pm->RegisterOperationType(CKOGUID_ARC_COSINE, "Arc Cosine");
    pm->RegisterOperationType(CKOGUID_ADDITION, "Addition");
    pm->RegisterOperationType(CKOGUID_SUBTRACTION, "Subtraction");
    pm->RegisterOperationType(CKOGUID_MULTIPLICATION, "Multiplication");
    pm->RegisterOperationType(CKOGUID_SPHERIC_TO_CARTESIAN, "Spheric To Cartesian");
    pm->RegisterOperationType(CKOGUID_DIVISION, "Division");
    pm->RegisterOperationType(CKOGUID_INFERIOR, "Inferior");
    pm->RegisterOperationType(CKOGUID_INFERIOR_OR_EQUAL, "Inferior or Equal");
    pm->RegisterOperationType(CKOGUID_MAX, "Max");
    pm->RegisterOperationType(CKOGUID_MIN, "Min");
    pm->RegisterOperationType(CKOGUID_EQUAL, "Equal");
    pm->RegisterOperationType(CKOGUID_NOT_EQUAL, "Not Equal");
    pm->RegisterOperationType(CKOGUID_IS_ACTIVE, "Is Active");
    pm->RegisterOperationType(CKOGUID_IS_CONTENT_EQUAL, "Is Content Equal");
    pm->RegisterOperationType(CKOGUID_CONVERT, "Convert");
    pm->RegisterOperationType(CKOGUID_SET_X, "Set X");
    pm->RegisterOperationType(CKOGUID_SET_Y, "Set Y");
    pm->RegisterOperationType(CKOGUID_SET_Z, "Set Z");
    pm->RegisterOperationType(CKOGUID_SET_W, "Set W");
    pm->RegisterOperationType(CKOGUID_SET_WIDTH, "Set Width");
    pm->RegisterOperationType(CKOGUID_SET_HEIGHT, "Set Height");
    pm->RegisterOperationType(CKOGUID_SET_LEFT, "Set Left");
    pm->RegisterOperationType(CKOGUID_SET_TOP, "Set Top");
    pm->RegisterOperationType(CKOGUID_SET_RIGHT, "Set Right");
    pm->RegisterOperationType(CKOGUID_SET_BOTTOM, "Set Bottom");
    pm->RegisterOperationType(CKOGUID_SET_RED, "Set Red");
    pm->RegisterOperationType(CKOGUID_SET_GREEN, "Set Green");
    pm->RegisterOperationType(CKOGUID_SET_BLUE, "Set Blue");
    pm->RegisterOperationType(CKOGUID_SET_ALPHA, "Set Alpha");
    pm->RegisterOperationType(CKOGUID_SET_GEOMETRIC_CENTER, "Set Geometric Center");
    pm->RegisterOperationType(CKOGUID_DEGRE_TO_RADIAN, "Degre To Radian");
    pm->RegisterOperationType(CKOGUID_RADIAN_TO_DEGRE, "Radian To Degre");
    pm->RegisterOperationType(CKOGUID_GET_X, "Get X");
    pm->RegisterOperationType(CKOGUID_GET_Y, "Get Y");
    pm->RegisterOperationType(CKOGUID_GET_Z, "Get Z");
    pm->RegisterOperationType(CKOGUID_GET_W, "Get W");
    pm->RegisterOperationType(CKOGUID_GET_RED, "Get Red");
    pm->RegisterOperationType(CKOGUID_GET_GREEN, "Get Green");
    pm->RegisterOperationType(CKOGUID_GET_BLUE, "Get Blue");
    pm->RegisterOperationType(CKOGUID_GET_ALPHA, "Get Alpha");
    pm->RegisterOperationType(CKOGUID_RANDOM, "Random");
    pm->RegisterOperationType(CKOGUID_AND, "And");
    pm->RegisterOperationType(CKOGUID_OR, "Or");
    pm->RegisterOperationType(CKOGUID_XOR, "Xor");
    pm->RegisterOperationType(CKOGUID_NOT, "Not");
    pm->RegisterOperationType(CKOGUID_INVERSE, "Inverse");
    pm->RegisterOperationType(CKOGUID_GET_NAME, "Get Name");
    pm->RegisterOperationType(CKOGUID_GET_ID, "Get ID");
    pm->RegisterOperationType(CKOGUID_GET_COLUMN_COUNT, "Get Column Count");
    pm->RegisterOperationType(CKOGUID_GET_ROW_COUNT, "Get Row Count");
    pm->RegisterOperationType(CKOGUID_GET_OBJECT_BY_NAME, "Get Object By Name");
    pm->RegisterOperationType(CKOGUID_GET_GEOMETRIC_CENTER, "Get Geometric Center");
    pm->RegisterOperationType(CKOGUID_GET_BOUNDING_BOX, "Get Bounding Box");
    pm->RegisterOperationType(CKOGUID_GET_HIERARCHICAL_BOUNDING_BOX, "Get Hierarchical Bounding Box");
    pm->RegisterOperationType(CKOGUID_GET_DIFFUSE_COLOR, "Get Diffuse Color");
    pm->RegisterOperationType(CKOGUID_GET_AMBIENT_COLOR, "Get Ambient Color");
    pm->RegisterOperationType(CKOGUID_GET_SPECULAR_COLOR, "Get Specular Color");
    pm->RegisterOperationType(CKOGUID_GET_SPECULAR_COLOR_POWER, "Get Specular Color Power");
    pm->RegisterOperationType(CKOGUID_GET_EMISSIVE_COLOR, "Get Emissive Color");
    pm->RegisterOperationType(CKOGUID_GET_TEXTURE, "Get Texture");
    pm->RegisterOperationType(CKOGUID_GET_WIDTH, "Get Width");
    pm->RegisterOperationType(CKOGUID_GET_HEIGHT, "Get Height");
    pm->RegisterOperationType(CKOGUID_GET_SLOT_COUNT, "Get Slot Count");
    pm->RegisterOperationType(CKOGUID_GET_MATERIAL, "Get Material");
    pm->RegisterOperationType(CKOGUID_GET_PARENT, "Get Parent");
    pm->RegisterOperationType(CKOGUID_GET_CHILDREN, "Get Children");
    pm->RegisterOperationType(CKOGUID_GET_REFERENTIEL_PLACE, "Get Referentiel Place");
    pm->RegisterOperationType(CKOGUID_GET_DISTANCE, "Get Distance");
    pm->RegisterOperationType(CKOGUID_IS_COLLISION, "Is Collision");
    pm->RegisterOperationType(CKOGUID_GET_WORLD_MATRIX, "Get World Matrix");
    pm->RegisterOperationType(CKOGUID_GET_LOCAL_MATRIX, "Get Local Matrix");
    pm->RegisterOperationType(CKOGUID_GET_TARGET, "Get Target");
    pm->RegisterOperationType(CKOGUID_PICK_OBJECT, "Pick Object");
    pm->RegisterOperationType(CKOGUID_GET_RANGE, "Get Range");
    pm->RegisterOperationType(CKOGUID_GET_FIELD_OF_VIEW, "Get Field Of View");
    pm->RegisterOperationType(CKOGUID_GET_NEAR_CLIP, "Get Near Clip");
    pm->RegisterOperationType(CKOGUID_GET_FAR_CLIP, "Get Far Clip");
    pm->RegisterOperationType(CKOGUID_GET_ORTHOGRAPHIC_ZOOM, "Get Orthographic Zoom");
    pm->RegisterOperationType(CKOGUID_GET_CHARACTER, "Get Character");
    pm->RegisterOperationType(CKOGUID_GET_ANIMATION, "Get Animation");
    pm->RegisterOperationType(CKOGUID_GET_ANIMATION_COUNT, "Get Animation Count");
    pm->RegisterOperationType(CKOGUID_GET_ROOT, "Get Root");
    pm->RegisterOperationType(CKOGUID_GET_BODYPARTS, "Get BodyParts");
    pm->RegisterOperationType(CKOGUID_GET_KINEMATIC_CHAIN, "Get Kinematic Chain");
    pm->RegisterOperationType(CKOGUID_GET_LENGTH, "Get Length");
    pm->RegisterOperationType(CKOGUID_GET_CURVE_POSITION, "Get Curve Position");
    pm->RegisterOperationType(CKOGUID_GET_CURVE_TANGENT, "Get Curve Tangent");
    pm->RegisterOperationType(CKOGUID_GET_GROUP_TYPE, "Get Group Type");
    pm->RegisterOperationType(CKOGUID_GET_TEXT, "Get Text");
    pm->RegisterOperationType(CKOGUID_GET_SIZE, "Get Size");
    pm->RegisterOperationType(CKOGUID_GET_TYPE, "Get Type");
    pm->RegisterOperationType(CKOGUID_GET_COLOR, "Get Color");
    pm->RegisterOperationType(CKOGUID_GET_POSITION, "Get Position");
    pm->RegisterOperationType(CKOGUID_GET_EULER_ANGLES, "Get Euler Angles");
    pm->RegisterOperationType(CKOGUID_GET_MATRIX, "Get Matrix");
    pm->RegisterOperationType(CKOGUID_GET_DIR, "Get Dir");
    pm->RegisterOperationType(CKOGUID_GET_UP, "Get Up");
    pm->RegisterOperationType(CKOGUID_GET_RIGHT, "Get Right");
    pm->RegisterOperationType(CKOGUID_GET_ELEMENT, "Get Element");
    pm->RegisterOperationType(CKOGUID_GET_RADIUS, "Get Radius");
    pm->RegisterOperationType(CKOGUID_GET_COUNT, "Get Count");
    pm->RegisterOperationType(CKOGUID_CROSS_PRODUCT, "Cross Product");
    pm->RegisterOperationType(CKOGUID_DOT_PRODUCT, "Dot Product");
    pm->RegisterOperationType(CKOGUID_GET_SCREEN_ORIGIN, "Get Screen Origin");
    pm->RegisterOperationType(CKOGUID_SUPERIOR, "Superior");
    pm->RegisterOperationType(CKOGUID_SUPERIOR_OR_EQUAL, "Superior or Equal");
    pm->RegisterOperationType(CKOGUID_IS_DERIVED_FROM, "Is Derived From");
    pm->RegisterOperationType(CKOGUID_IS_VISIBLE, "Is Visible");
    pm->RegisterOperationType(CKOGUID_OPPOSITE, "Opposite");
    pm->RegisterOperationType(CKOGUID_RAINBOW_COLOR, "Rainbow Color");
    pm->RegisterOperationType(CKOGUID_GET_ANGLE, "Get Angle");
    pm->RegisterOperationType(CKOGUID_NORMALIZE, "Normalize");
    pm->RegisterOperationType(CKOGUID_REFLECT, "Reflect");
    pm->RegisterOperationType(CKOGUID_ABSOLUTE_VALUE, "Absolute Value");
    pm->RegisterOperationType(CKOGUID_IS_IN, "Is In");
    pm->RegisterOperationType(CKOGUID_GET_CURRENT, "Get Current");
    pm->RegisterOperationType(CKOGUID_GET_MAGNITUDE, "Get Magnitude");
    pm->RegisterOperationType(CKOGUID_GET_SCALE, "Get Scale");
    pm->RegisterOperationType(CKOGUID_GET_BODYPART, "Get BodyPart");
    pm->RegisterOperationType(CKOGUID_CONTAIN_STRING, "Contain String");
    pm->RegisterOperationType(CKOGUID_GET_SCRIPT_COUNT, "Get Script Count");
    pm->RegisterOperationType(CKOGUID_GET_SCRIPT, "Get Script");
    pm->RegisterOperationType(CKOGUID_GET_VIEW_RECT, "Get View Rect");
    pm->RegisterOperationType(CKOGUID_IS_CHILD_OF, "Is Child Of");
    pm->RegisterOperationType(CKOGUID_IS_BODY_PART_OF, "Is Body Part Of");
    pm->RegisterOperationType(CKOGUID_TRANSFORM, "Transform");
    pm->RegisterOperationType(CKOGUID_TRANSFORM_VECTOR, "Transform Vector");
    pm->RegisterOperationType(CKOGUID_INVERSE_TRANSFORM, "Inverse Transform");
    pm->RegisterOperationType(CKOGUID_INVERSE_TRANSFORM_VECTOR, "Inverse Transform Vector");
    pm->RegisterOperationType(CKOGUID_DYNAMIC_CAST, "Dynamic Cast");
    pm->RegisterOperationType(CKOGUID_SYMMETRY, "Symmetry");
    pm->RegisterOperationType(CKOGUID_GET_MESH_LIST, "Get Mesh List");
    pm->RegisterOperationType(CKOGUID_GET_MATERIAL_LIST, "Get Material List");
    pm->RegisterOperationType(CKOGUID_GET_ASPECT_RATIO, "Get Aspect Ratio");
    pm->RegisterOperationType(CKOGUID_GET_VERTEX_COUNT, "Get Vertex Count");
    pm->RegisterOperationType(CKOGUID_GET_VERTEX_COLOR, "Get Vertex Color");
    pm->RegisterOperationType(CKOGUID_GET_VERTEX_POSITION, "Get Vertex Position");
    pm->RegisterOperationType(CKOGUID_GET_VERTEX_SPECULAR_COLOR, "Get Vertex Specular Color");
    pm->RegisterOperationType(CKOGUID_GET_VERTEX_NORMAL, "Get Vertex Normal");
    pm->RegisterOperationType(CKOGUID_GET_VERTEX_UVS, "Get Vertex UVs");
    pm->RegisterOperationType(CKOGUID_GET_MATERIAL_COUNT, "Get Material Count");
    pm->RegisterOperationType(CKOGUID_GET_CHANNEL_COUNT, "Get Channel Count");
    pm->RegisterOperationType(CKOGUID_GET_CHANNEL_BY_MATERIAL, "Get Channel By Material");
    pm->RegisterOperationType(CKOGUID_GET_FACE_COUNT, "Get Face Count");
    pm->RegisterOperationType(CKOGUID_GET_PM_RENDERED_VERTICES_COUNT, "Get PM Rendered Vertices Count");
    pm->RegisterOperationType(CKOGUID_GET_FACE_MATERIAL, "Get Face Material");
    pm->RegisterOperationType(CKOGUID_GET_FACE_NORMAL, "Get Face Normal");
    pm->RegisterOperationType(CKOGUID_GET_FACE_VERTEX_INDICES, "Get Face Vertex Indices");
    pm->RegisterOperationType(CKOGUID_GET_VERTEX_WEIGHT, "Get Vertex Weight");
    pm->RegisterOperationType(CKOGUID_FROM_ROTATION, "From Rotation");
    pm->RegisterOperationType(CKOGUID_GET_PORTALS, "Get Portals");
    pm->RegisterOperationType(CKOGUID_GET_PLACE, "Get Place");
    pm->RegisterOperationType(CKOGUID_GET_PLAYED_TIME, "Get Played Time");
    pm->RegisterOperationType(CKOGUID_GET_SOUND_FILE_NAME, "Get Sound file Name");
    pm->RegisterOperationType(CKOGUID_GET_SAMPLING_RATE, "Get Sampling Rate");
    pm->RegisterOperationType(CKOGUID_GET_PITCH, "Get Pitch");
    pm->RegisterOperationType(CKOGUID_GET_GAIN, "Get Gain");
    pm->RegisterOperationType(CKOGUID_IS_LOOPING, "Is Looping");
    pm->RegisterOperationType(CKOGUID_IS_STREAMING, "Is Streaming");
    pm->RegisterOperationType(CKOGUID_IS_PLAYING, "Is Playing");
    pm->RegisterOperationType(CKOGUID_IS_PAUSED, "Is Paused");
    pm->RegisterOperationType(CKOGUID_GET_PAN, "Get Pan");
    pm->RegisterOperationType(CKOGUID_GET_RELATIVE_POSITION, "Get Relative Position");
    pm->RegisterOperationType(CKOGUID_GET_RELATIVE_DIRECTION, "Get Relative Direction");
    pm->RegisterOperationType(CKOGUID_GET_CONE, "Get Cone");
    pm->RegisterOperationType(CKOGUID_GET_MIN_MAX, "Get Min Max");
    pm->RegisterOperationType(CKOGUID_GET_VELOCITY, "Get Velocity");
    pm->RegisterOperationType(CKOGUID_GET_BOTTOM_RIGHT_CORNER, "Get Bottom-Right Corner");
    pm->RegisterOperationType(CKOGUID_PER_SECOND, "Per Second");
    pm->RegisterOperationType(CKOGUID_SET_POSITION, "Set Position");
    pm->RegisterOperationType(CKOGUID_GET_IN_TANGENT, "Get In Tangent");
    pm->RegisterOperationType(CKOGUID_GET_OUT_TANGENT, "Get Out Tangent");
}

void CKInitializeOperationFunctions(CKContext *context);

CKERROR ParamOp_InitInstance(CKContext *context)
{
    CKInitializeOperationTypes(context);
    CKInitializeOperationFunctions(context);

    return CK_OK;
}

PLUGIN_EXPORT CKPluginInfo *CKGetPluginInfo(int Index)
{
    g_PluginInfo.m_Author = "Virtools";
    g_PluginInfo.m_Description = "Parameters Operations";
    g_PluginInfo.m_Extension = "";
    g_PluginInfo.m_Type = CKPLUGIN_MANAGER_DLL;
    g_PluginInfo.m_Version = 0x000001;
    g_PluginInfo.m_InitInstanceFct = ParamOp_InitInstance;
    g_PluginInfo.m_ExitInstanceFct = NULL;
    g_PluginInfo.m_GUID = PARAMOP_GUID;
    g_PluginInfo.m_Summary = ParamOpName;
    return &g_PluginInfo;
}
