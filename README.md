
#  SecureWipe: Cross-Platform Secure Data Wiping Solution

##  Overview
SecureWipe is an open-source, cross-platform data wiping system focused on verifiable and standards-compliant data erasure across a wide range of storage devices.

It supports HDDs, SSDs, NVMe drives, removable USB storage, and Android devices, and is designed to perform complete, irreversible data destruction, not just filesystem-level deletion.

SecureWipe runs on a 64-bit Tiny Core Linux base to keep the environment minimal, fast, and hardware-tolerant, making it usable on both modern systems and older machines without relying on vendor-specific tooling.

The project follows NIST SP 800-88 data sanitization guidelines and produces cryptographically verifiable wipe records that can be audited or validated later. These records support reproducible erasure workflows, compliance verification, and responsible device reuse or disposal.

---

##  Key Features

-  **Full Disk Wipe** — ATA Secure Erase for HDDs, NVMe Secure Erase for SSDs

-  **Android Wipe** — Automated ADB + Fastboot reset for complete data removal

-  **USB Wipe** — File-system signature wiping using the `dd` protocol

-  **Tamper-Proof Certificates** — Verifiable proof of secure erasure

-  **Offline Operation** — Works entirely without internet access

-  **Lightweight** — Tiny Core Linux base for minimal system resource usage

---

##  Installation & Usage Guide

###  For Windows Users

#### Step 1 — Download Files

1. Visit the [official SecureWipe website](https://devpoliooo.vercel.app/).

2. Download the latest **SecureWipe ISO** file.

#### Step 2 — Create a Bootable USB with Rufus

1. Download and install [**Rufus**](https://rufus.ie).

2. Insert a USB drive (minimum 4 GB).

3. Open **Rufus** and configure:

- **Device:** Select your USB drive

- **Boot selection:** Choose the downloaded `SecureWipe.iso`

- **Partition scheme:** Choose `MBR` (Legacy BIOS) or `GPT` (UEFI) based on your device.

4. Click **Start** and wait for Rufus to finish creating the bootable USB.

#### Step 3 — Boot into SecureWipe

1. Insert the USB drive into the target computer.

2. Restart and open the boot menu (`F12`, `Esc`, or `Del` depending on the system).

3. Select your USB device to boot into **SecureWipe**.

#### Step 4 — Start Wiping

Once the SecureWipe interface loads, select an option:

-  **Wipe Internal Disk (HDD/NVMe)**

-  **Wipe USB Devices**

-  **Wipe Android Devices**

Follow the on-screen prompts.
After completion, SecureWipe automatically generates a **wipe certificate** which will be stored on the Pendrive.

---

###  For Linux Users

#### Option 1 — Using the ISO File

1. Download the `SecureWipe.iso` file.

2. Identify your USB drive (e.g., `/dev/sdb`).

3. Use the `dd` command to make a bootable USB:

```bash

sudo dd if=SecureWipe.iso of=/dev/sdX bs=4M status=progress conv=fsync
```

Replace /dev/sdX with your actual USB device path (not a partition like /dev/sdb1).
Reboot your system and select the USB device from the boot menu.

---

### Option 2 — Using the .deb Package

Download the securewipe.deb package from the official website.
Install it using:

```bash
sudo dpkg -i securewipe.deb
sudo apt-get install -f
```

Launch SecureWipe with:

```bash
securewipe
```

Follow the interface to securely wipe supported devices.

### For Android devices:

- Enable USB Debugging in Developer Options before connecting.
- Ensure the phone is properly recognized by ADB.


### Wipe Certificates

After each successful operation, SecureWipe creates a digital wipe certificate that includes:
- Device name and serial number
- Wipe timestamp
- Wipe method (ATA, NVMe, ADB, or dd)
- Verification checksum
- Operator ID (if configured)

These certificates act as tamper-proof audit records, providing verifiable proof of data erasure for organizations and compliance audits.

### !!! Important Notes

 - All data will be permanently erased. Back up any critical data before proceeding. 
 - Use SecureWipe only on devices you own or are authorized to wipe.
 - Some older HDDs may not support ATA Secure Erase commands. Guttman method will be employed which may take some time.
