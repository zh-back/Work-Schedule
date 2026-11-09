#include "test_framework.h"
#include <windows.h>
#include <stdio.h>
#include "WebView2.h"

int test_webview2_sdk_symbols(void) {
    HMODULE hLoader = LoadLibraryA("WebView2Loader.dll");
    if (!hLoader) {
        hLoader = LoadLibraryA("lib\\WebView2Loader.dll");
    }
    ASSERT_TRUE(hLoader != NULL);

    auto pfn = (decltype(&CreateCoreWebView2EnvironmentWithOptions))GetProcAddress(hLoader, "CreateCoreWebView2EnvironmentWithOptions");
    ASSERT_TRUE(pfn != NULL);

    FreeLibrary(hLoader);
    return 0;
}

int main(void) {
    printf("=== Running WebView2 SDK & Loader Tests ===\n");
    RUN_TEST(test_webview2_sdk_symbols);
    TEST_REPORT();
}
