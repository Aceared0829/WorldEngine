#pragma once

/// Operator to serialize WIAllocator::Stats objects.
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, const WAllocator::Stats& rhs);

/// Operator to serialize WIAllocator::Stats objects.
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WAllocator::Stats& rhs);

struct WTime;

/// Operator to serialize WTime objects.
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, WTime value);

/// Operator to serialize WTime objects.
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WTime& ref_value);


class WUuid;

/// Operator to serialize WUuid objects. [tested]
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, const WUuid& value);

/// Operator to serialize WUuid objects. [tested]
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WUuid& ref_value);

class WHashedString;

/// Operator to serialize WHashedString objects. [tested]
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, const WHashedString& sValue);

/// Operator to serialize WHashedString objects. [tested]
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WHashedString& ref_sValue);

class WTempHashedString;

/// Operator to serialize WHashedString objects.
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, const WTempHashedString& sValue);

/// Operator to serialize WHashedString objects.
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WTempHashedString& ref_sValue);

class WVariant;

/// Operator to serialize WVariant objects.
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, const WVariant& value);

/// Operator to serialize WVariant objects.
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WVariant& ref_value);

class WTimestamp;

/// Operator to serialize WTimestamp objects.
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, WTimestamp value);

/// Operator to serialize WTimestamp objects.
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WTimestamp& ref_value);

struct WVarianceTypeFloat;

/// Operator to serialize WTimestamp objects.
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, const WVarianceTypeFloat& value);

/// Operator to serialize WTimestamp objects.
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WVarianceTypeFloat& ref_value);

struct WVarianceTypeTime;

/// Operator to serialize WTimestamp objects.
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, const WVarianceTypeTime& value);

/// Operator to serialize WTimestamp objects.
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WVarianceTypeTime& ref_value);

struct WVarianceTypeAngle;

/// Operator to serialize WTimestamp objects.
W_FOUNDATION_DLL void operator<<(WStreamWriter& inout_stream, const WVarianceTypeAngle& value);

/// Operator to serialize WTimestamp objects.
W_FOUNDATION_DLL void operator>>(WStreamReader& inout_stream, WVarianceTypeAngle& ref_value);
