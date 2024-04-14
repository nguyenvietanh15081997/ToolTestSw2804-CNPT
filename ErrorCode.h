#pragma once

// TODO: add to all Do function
// TODO: Merge MQTT error code
typedef enum
{
	CODE_ERROR = -1,
	CODE_OK = 0,
	CODE_NOT_RESPONSE,
	CODE_DATA_ARRAY,
	CODE_TIMEOUT,
	CODE_NOT_FOUND_DEVICE,
	CODE_NOT_FOUND_GROUP,
	CODE_NOT_FOUND_SCENE,
	CODE_NOT_FOUND_RULE,
	CODE_NOT_FOUND_ROOM,
	CODE_FORMAT_ERROR,
	CODE_DATABASE_ERROR,
	CODE_MEMORY_ERROR,
	CODE_FILE_ERROR,
	CODE_EXIT,
	CODE_FACTORY,
} ErrorCode;
