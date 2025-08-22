#pragma once

constexpr auto ConfigSchema = R"(
{
    "type": "object",
    "properties": {
        "mapping_directory": {
            "type": "string"
        },
        "schemas_directory": {
            "type": "string"
        },
        "trace_enabled": {
            "type": "boolean"
        },
        "cache_enabled": {
            "type": "boolean"
        },
        "cache_size": {
            "type": "integer",
            "minimum": 0
        },
        "custom_function_libraries": {
            "type": "array",
            "items": {
                "type": "string"
            }
        }
    },
    "required": ["mapping_directory", "schemas_directory"],
    "additionalProperties": false
}
)";
