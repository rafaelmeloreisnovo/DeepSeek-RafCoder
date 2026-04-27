plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.rafcoder.app"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.rafcoder.app"
        minSdk = 24
        targetSdk = 35
        versionCode = 1
        versionName = "1.0.0"

        ndk {
            abiFilters += listOf("armeabi-v7a", "arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++17", "-O2")
            }
        }
    }

    signingConfigs {
        create("release") {
            val storeFilePath = providers.environmentVariable("ANDROID_KEYSTORE_PATH").orNull
            val storePass = providers.environmentVariable("ANDROID_KEYSTORE_PASSWORD").orNull
            val keyAlias = providers.environmentVariable("ANDROID_KEY_ALIAS").orNull
            val keyPass = providers.environmentVariable("ANDROID_KEY_PASSWORD").orNull
            if (!storeFilePath.isNullOrBlank() && !storePass.isNullOrBlank() && !keyAlias.isNullOrBlank() && !keyPass.isNullOrBlank()) {
                storeFile = file(storeFilePath)
                storePassword = storePass
                this.keyAlias = keyAlias
                keyPassword = keyPass
            }
        }
    }

    buildTypes {
        debug {
            isMinifyEnabled = false
        }
        release {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            if (providers.environmentVariable("ANDROID_KEYSTORE_PATH").isPresent) {
                signingConfig = signingConfigs.getByName("release")
            }
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = false
        }
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("com.google.android.material:material:1.12.0")
}
