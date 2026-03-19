/* Auto-generated model: incident */
#include "core/active_record.h"
#include "core/active_model.h"

#define INCIDENT_MODEL_NAME "incident"

typedef ActiveModel Incident;

/* Fields: use active_model_set_field(m, "key", "value"); */

Incident *incident_create(ActiveRecordStore *store) {
    return active_record_create(store, INCIDENT_MODEL_NAME);
}
