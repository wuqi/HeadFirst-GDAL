#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

START_TEST(test_buffer_overflow_protection)
{
    // Invariant: Buffer reads never exceed the declared length
    const char *payloads[] = {
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",  // 256 bytes - exact exploit
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",  // 128 bytes - boundary
        "valid_input.sub"  // valid input
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process: execute sub2tif with payload
            char *args[] = {"./sub2tif", (char*)payloads[i], NULL};
            execv("./sub2tif", args);
            exit(1);
        } else if (pid > 0) {
            // Parent process: wait and check for crash
            int status;
            waitpid(pid, &status, 0);
            
            // Assert: process should not crash (SIGSEGV, SIGABRT)
            if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                ck_assert_msg(sig != SIGSEGV && sig != SIGABRT,
                    "Buffer overflow detected: process crashed with signal %d for payload %d", sig, i);
            }
        } else {
            ck_abort_msg("Fork failed");
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_overflow_protection);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}