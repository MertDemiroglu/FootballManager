#include "fm/common/DateUtils.h"

namespace DateUtils {
    Date addDays(Date date, int days) {
        for (int i = 0; i < days; ++i) {
            date.advanceDay();
        }
        return date;
    }
}