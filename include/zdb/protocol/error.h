#pragma once

enum class FramingError { not_enough_data, too_large_frame, malformed_data };

enum class DeserialisationError {
  invalid_object_representation,
  invalid_type,
  inconsistent_data
};

enum class SerialisationError { size_type_cast_error };
