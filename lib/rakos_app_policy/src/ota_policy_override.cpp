extern "C" {

bool verifyRollbackLater() {
    return true;
}

void __wrap_esp_ota_mark_app_valid_cancel_rollback(void) {}

}
