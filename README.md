# ARCore Android SDK

Welcome to the ARCore Android SDK! This SDK provides APIs for all the essential AR features like motion tracking, environmental understanding, and light estimation. With these capabilities, you can build entirely new AR experiences or enhance existing apps with AR features.

## Table of Contents

- Introduction
- Getting Started
- Installation
- Usage
- Examples
- Contributing
- License

## Introduction

ARCore is a platform for building augmented reality apps on Android. It uses three key capabilities to integrate virtual content with the real world as seen through your phone's camera:
- **Motion tracking**: Allows the phone to understand and track its position relative to the world.
- **Environmental understanding**: Allows the phone to detect the size and location of flat horizontal surfaces like the ground or a coffee table.
- **Light estimation**: Allows the phone to estimate the environment's current lighting conditions.

## Getting Started

To get started with ARCore, you'll need to:
1. Install Android Studio.
2. Download the ARCore SDK for Android.
3. Set up your development environment.

## Installation

To install the ARCore SDK, follow these steps:
1. Clone the repository:
    ```bash
    git clone https://github.com/google-ar/arcore-android-sdk.git
    ```
2. Open the project in Android Studio.
3. Sync the project with Gradle files.

## Usage

Here's a basic example of how to use the ARCore SDK in your project:

```java
import com.google.ar.core.*;

public class HelloARActivity extends AppCompatActivity {
    private Session session;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Create a new ARCore session.
        session = new Session(this);

        // Configure the session.
        Config config = new Config(session);
        session.configure(config);
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Resume the AR session.
        session.resume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        // Pause the AR session.
        session.pause();
    }
}

Examples
Check out the examples directory for more sample projects demonstrating how to use the ARCore SDK.

Contributing
We welcome contributions! Please see CONTRIBUTING.md for details on how to get started.

License
This project is licensed under the Apache 2.0 License - see the LICENSE file for details.


Feel free to adjust the sections and content as needed for your project. If you have any specific requirements or additional sections you'd like to include, let me know!ARCore SDK for Android
======================
Copyright 2017 Google LLC

This SDK provides APIs for all of the essential AR features like motion
tracking, environmental understanding, and light estimation. With these
capabilities you can build entirely new AR experiences or enhance existing apps
with AR features.


## Breaking change affecting previously published 32-bit-only apps

_Google Play Services for AR_ (ARCore) has removed support for 32-bit-only
ARCore-enabled apps running on 64-bit devices. Support for 32-bit apps running
on 32-bit devices is unaffected.

If you have published a 32-bit-only (`armeabi-v7a`) version of your
ARCore-enabled app without publishing a corresponding 64-bit (`arm64-v8a`)
version, you must update your app to include 64-bit native libraries.
32-bit-only ARCore-enabled apps that are not updated by this time may crash when
attempting to start an augmented reality (AR) session.

To learn more about this breaking change, and for instructions on how to update
your app, see https://developers.google.com/ar/64bit.


## Quick Start

See the [Quickstart for Android Java](//developers.google.com/ar/develop/java/quickstart)
or [Quickstart for Android NDK](//developers.google.com/ar/develop/c/quickstart)
developer guide.


## API Reference

See the [ARCore SDK for Java API Reference](//developers.google.com/ar/reference/java)
or [ARCore SDK for C API Reference](//developers.google.com/ar/reference/c).


## Release Notes

The SDK release notes are available on the
[releases](//github.com/google-ar/arcore-android-sdk/releases) page.


## Terms & Conditions

By downloading the ARCore SDK for Android, you agree that the
[**ARCore Additional Terms of Service**](https://developers.google.com/ar/develop/terms)
governs your use thereof.


## User privacy requirements

You must disclose the use of Google Play Services for AR (ARCore) and how it
collects and processes data, prominently in your application, easily accessible
to users. You can do this by adding the following text on your main menu or
notice screen: "This application runs on [Google Play Services for AR](//play.google.com/store/apps/details?id=com.google.ar.core) (ARCore),
which is provided by Google LLC and governed by the [Google Privacy Policy](//policies.google.com/privacy)".

See the [User privacy requirements](https://developers.google.com/ar/develop/privacy-requirements).

## Deprecation policy

Apps built with **ARCore SDK 1.12.0 or higher** are covered by the
[Cloud Anchor API deprecation policy](//developers.google.com/ar/distribute/deprecation-policy).

Apps built with **ARCore SDK 1.11.0 or lower** will be unable to host or resolve
Cloud Anchors beginning December 2020 due to the SDK's use of an older,
deprecated ARCore Cloud Anchor service.
