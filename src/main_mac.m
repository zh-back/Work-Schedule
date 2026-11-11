#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "server.h"
#include "db.h"
#include <unistd.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, WKNavigationDelegate, WKUIDelegate>
@property (strong, nonatomic) NSWindow *window;
@property (strong, nonatomic) WKWebView *webView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSRect screenRect = [[NSScreen mainScreen] visibleFrame];
    CGFloat width = 1120;
    CGFloat height = 760;
    NSRect frame = NSMakeRect((screenRect.size.width - width) / 2,
                              (screenRect.size.height - height) / 2,
                              width, height);

    NSWindowStyleMask style = NSWindowStyleMaskTitled |
                              NSWindowStyleMaskClosable |
                              NSWindowStyleMaskMiniaturizable |
                              NSWindowStyleMaskResizable |
                              NSWindowStyleMaskFullSizeContentView;

    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Schedule & WorkLog"];
    self.window.titlebarAppearsTransparent = YES;
    self.window.titleVisibility = NSWindowTitleHidden;
    [self.window setMinSize:NSMakeSize(820, 560)];

    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
    self.webView = [[WKWebView alloc] initWithFrame:self.window.contentView.bounds configuration:config];
    self.webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.webView.navigationDelegate = self;
    self.webView.UIDelegate = self;

    [self.window.contentView addSubview:self.webView];

    int port = server_get_bound_port();
    NSString *urlString = [NSString stringWithFormat:@"http://127.0.0.1:%d", port];
    NSURL *url = [NSURL URLWithString:urlString];
    [self.webView loadRequest:[NSURLRequest requestWithURL:url]];

    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    server_stop();
    db_close();
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // 1. Resolve storage directory in ~/Library/Application Support/ScheduleWorklog
        NSString *appSupport = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject];
        NSString *dataDir = [appSupport stringByAppendingPathComponent:@"ScheduleWorklog"];
        [[NSFileManager defaultManager] createDirectoryAtPath:dataDir withIntermediateDirectories:YES attributes:nil error:nil];
        NSString *dbPath = [dataDir stringByAppendingPathComponent:@"schedule_worklog.db"];

        // 2. Change working directory to Resources so "web/" static assets can be loaded
        NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
        if (resourcePath) {
            chdir([resourcePath UTF8String]);
        }

        // 3. Initialize SQLite database & background embedded server
        db_init([dbPath UTF8String]);
        server_start_background(0);

        // 4. Start Cocoa application
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];

        AppDelegate *delegate = [[AppDelegate alloc] init];
        app.delegate = delegate;

        // 5. Construct macOS native menu bar
        NSMenu *menubar = [[NSMenu alloc] init];
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
        [menubar addItem:appMenuItem];
        [app setMainMenu:menubar];

        NSMenu *appMenu = [[NSMenu alloc] init];
        NSString *appName = @"Schedule & WorkLog";
        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"退出 %@", appName]
                                                              action:@selector(terminate:)
                                                       keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        [appMenuItem setSubmenu:appMenu];

        // Edit menu for standard Cmd+C / Cmd+V / Cmd+A / Cmd+Z shortcuts in WebView
        NSMenuItem *editMenuItem = [[NSMenuItem alloc] init];
        [menubar addItem:editMenuItem];
        NSMenu *editMenu = [[NSMenu alloc] initWithTitle:@"编辑"];
        [editMenu addItemWithTitle:@"撤销" action:@selector(undo:) keyEquivalent:@"z"];
        [editMenu addItemWithTitle:@"重做" action:@selector(redo:) keyEquivalent:@"Z"];
        [editMenu addItem:[NSMenuItem separatorItem]];
        [editMenu addItemWithTitle:@"剪切" action:@selector(cut:) keyEquivalent:@"x"];
        [editMenu addItemWithTitle:@"复制" action:@selector(copy:) keyEquivalent:@"c"];
        [editMenu addItemWithTitle:@"粘贴" action:@selector(paste:) keyEquivalent:@"v"];
        [editMenu addItemWithTitle:@"全选" action:@selector(selectAll:) keyEquivalent:@"a"];
        [editMenuItem setSubmenu:editMenu];

        [app run];
    }
    return 0;
}
