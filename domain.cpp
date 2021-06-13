#include "domain.h"

namespace transport_catalogue::domain {
    bool Bus::operator==(const Bus& other) const {
        return this->number == other.number;
    }
}