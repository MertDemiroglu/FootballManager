#pragma once

#include "fm/common/Date.h"
#include "fm/data/SaveMetadata.h"

#include <string>

struct SaveValidationResult {
    bool valid = false;
    std::string reason;
    Date currentDate{ 1900, Month::January, 1 };
};

class RuntimeSaveValidator {
public:
    static SaveValidationResult validateExistingSave(
        const std::string& databasePath,
        const SaveMetadata* metadata = nullptr);

    static SaveValidationResult validateNewSave(
        const std::string& databasePath,
        const SaveMetadata& metadata,
        const Date& expectedInitialDate);
};
