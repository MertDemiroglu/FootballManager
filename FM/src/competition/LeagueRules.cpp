#include"fm/competition/LeagueRules.h"
namespace {
    bool dateLess(const Date& lhs, const Date& rhs) {
        return lhs < rhs;
    }
}

bool TransferWindow::contains(const Date& date) const {
    return !dateLess(date, start) && dateLess(date, endExclusive);
}
