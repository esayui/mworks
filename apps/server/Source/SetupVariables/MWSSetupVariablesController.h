//
//  MWSSetupVariablesController.h
//  MWServer
//
//  Created by Christopher Stawarz on 6/4/14.
//
//

#import <Foundation/Foundation.h>


@interface MWSSetupVariablesController : NSObject {
    dispatch_queue_t writeQueue;
    
    NSString *_serverName;
    NSNumber *_displayWidth;
    NSNumber *_displayHeight;
    NSNumber *_displayDistance;
    BOOL _alwaysDisplayMirrorWindow;
    NSNumber *_mirrorWindowBaseHeight;
    BOOL _redrawOnEveryRefresh;
    BOOL _announceIndividualStimuli;
    BOOL _warnOnSkippedRefresh;
    BOOL _allowAltFailover;
    BOOL _useHighPrecisionClock;
}

@property(nonatomic, copy) NSString *serverName;
@property(nonatomic, strong) NSNumber *displayWidth;
@property(nonatomic, strong) NSNumber *displayHeight;
@property(nonatomic, strong) NSNumber *displayDistance;
@property(nonatomic) BOOL alwaysDisplayMirrorWindow;
@property(nonatomic, strong) NSNumber *mirrorWindowBaseHeight;
@property(nonatomic) BOOL redrawOnEveryRefresh;
@property(nonatomic) BOOL announceIndividualStimuli;
@property(nonatomic) BOOL warnOnSkippedRefresh;
@property(nonatomic) BOOL allowAltFailover;
@property(nonatomic) BOOL useHighPrecisionClock;

- (instancetype)init;

@end