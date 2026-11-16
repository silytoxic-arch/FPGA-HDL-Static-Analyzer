#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#import <Vision/Vision.h>

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        if (argc != 3) {
            fprintf(stderr, "usage: ocr_pdf_pages <page-directory> <output-text>\n");
            return 2;
        }

        NSFileManager *fileManager = NSFileManager.defaultManager;
        NSURL *pageDirectory = [NSURL fileURLWithPath:@(argv[1]) isDirectory:YES];
        NSError *error = nil;
        NSArray<NSURL *> *allURLs = [fileManager contentsOfDirectoryAtURL:pageDirectory
                                               includingPropertiesForKeys:nil
                                                                  options:0
                                                                    error:&error];
        if (!allURLs) {
            fprintf(stderr, "unable to list pages: %s\n", error.localizedDescription.UTF8String);
            return 1;
        }

        NSPredicate *isPNG = [NSPredicate predicateWithBlock:^BOOL(NSURL *url, NSDictionary *bindings) {
            return [url.pathExtension.lowercaseString isEqualToString:@"png"];
        }];
        NSArray<NSURL *> *pageURLs = [[allURLs filteredArrayUsingPredicate:isPNG]
            sortedArrayUsingComparator:^NSComparisonResult(NSURL *left, NSURL *right) {
                return [left.lastPathComponent localizedStandardCompare:right.lastPathComponent];
            }];

        NSMutableString *output = NSMutableString.string;
        NSUInteger pageNumber = 0;
        for (NSURL *pageURL in pageURLs) {
            @autoreleasepool {
                pageNumber += 1;
                VNRecognizeTextRequest *request = [VNRecognizeTextRequest new];
                request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
                request.recognitionLanguages = @[@"en-US"];
                request.usesLanguageCorrection = YES;

                CGImageSourceRef imageSource = CGImageSourceCreateWithURL(
                    (__bridge CFURLRef)pageURL, NULL);
                CGImageRef image = imageSource
                    ? CGImageSourceCreateImageAtIndex(imageSource, 0, NULL)
                    : NULL;
                if (!image) {
                    fprintf(stderr, "unable to decode %s\n", pageURL.path.UTF8String);
                    if (imageSource) CFRelease(imageSource);
                    continue;
                }
                VNImageRequestHandler *handler = [[VNImageRequestHandler alloc]
                    initWithCGImage:image options:@{}];
                NSError *recognitionError = nil;
                if (![handler performRequests:@[request] error:&recognitionError]) {
                    fprintf(stderr, "OCR failed for %s: %s\n",
                            pageURL.path.UTF8String,
                            recognitionError.localizedDescription.UTF8String);
                    CGImageRelease(image);
                    CFRelease(imageSource);
                    continue;
                }
                CGImageRelease(image);
                CFRelease(imageSource);

                NSArray<VNRecognizedTextObservation *> *observations = [request.results
                    sortedArrayUsingComparator:^NSComparisonResult(
                        VNRecognizedTextObservation *left,
                        VNRecognizedTextObservation *right) {
                        CGFloat verticalDelta = fabs(CGRectGetMidY(left.boundingBox) -
                                                     CGRectGetMidY(right.boundingBox));
                        if (verticalDelta > 0.008) {
                            return CGRectGetMidY(left.boundingBox) > CGRectGetMidY(right.boundingBox)
                                ? NSOrderedAscending : NSOrderedDescending;
                        }
                        return CGRectGetMinX(left.boundingBox) < CGRectGetMinX(right.boundingBox)
                            ? NSOrderedAscending : NSOrderedDescending;
                    }];

                [output appendFormat:@"\n=== PAGE %lu %@ ===\n",
                                     (unsigned long)pageNumber,
                                     pageURL.lastPathComponent];
                for (VNRecognizedTextObservation *observation in observations) {
                    VNRecognizedText *candidate = [observation topCandidates:1].firstObject;
                    if (candidate) {
                        [output appendString:candidate.string];
                        [output appendString:@"\n"];
                    }
                }
                fprintf(stderr, "OCR %lu/%lu\r",
                        (unsigned long)pageNumber,
                        (unsigned long)pageURLs.count);
            }
        }

        NSURL *outputURL = [NSURL fileURLWithPath:@(argv[2])];
        if (![output writeToURL:outputURL atomically:YES encoding:NSUTF8StringEncoding error:&error]) {
            fprintf(stderr, "unable to write output: %s\n", error.localizedDescription.UTF8String);
            return 1;
        }
    }
    return 0;
}
