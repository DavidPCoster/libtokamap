#pragma once

constexpr auto ConfigSchema = R"(
{
    "type": "object",
    "properties": {
        "mapping_directory": {
            "type": "string"
        },
        "mapping_schema": {
            "type": "string"
        },
        "globals_schema": {
            "type": "string"
        },
        "mapping_config_schema": {
            "type": "string"
        },
        "cache_enabled": {
            "type": "boolean"
        },
        "cache_size": {
            "type": "integer",
            "minimum": 0
        },
        "custom_library_paths": {
            "type": "array",
            "items": {
                "type": "string"
            }
        }
    },
    "required": ["mapping_directory", "mapping_schema", "globals_schema", "mapping_config_schema"],
    "additionalProperties": false
}
)";
