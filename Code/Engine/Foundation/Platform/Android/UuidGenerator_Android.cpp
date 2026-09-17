#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_PLATFORM_ANDROID)

#  include <Foundation/Types/Uuid.h>

#  include <Foundation/Platform/Android/Utils/AndroidJni.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <android_native_app_glue.h>

WUuid WUuid::MakeUuid()
{
  WJniAttachment attachment;

  WJniClass uuidClass("java/util/UUID");
  W_ASSERT_DEBUG(!uuidClass.IsNull(), "UUID class not found.");
  WJniObject javaUuid = uuidClass.CallStatic<WJniObject>("randomUUID");
  jlong mostSignificant = javaUuid.Call<jlong>("getMostSignificantBits");
  jlong leastSignificant = javaUuid.Call<jlong>("getLeastSignificantBits");

  return WUuid(leastSignificant, mostSignificant);

  // #TODO maybe faster to read /proc/sys/kernel/random/uuid, but that can't be done via WOSFile
  //  see https://stackoverflow.com/questions/11888055/include-uuid-h-into-android-ndk-project
}

#endif
