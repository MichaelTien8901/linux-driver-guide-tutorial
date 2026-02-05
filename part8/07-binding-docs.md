---
layout: default
title: "8.7 Binding Documentation"
parent: "Part 8: Platform Bus and Device Tree"
nav_order: 7
---

# Binding Documentation

When submitting drivers to the Linux kernel, you must document your Device Tree binding. Modern bindings use YAML format with dt-schema validation.

## Binding File Location

```
Documentation/devicetree/bindings/
├── vendor-prefixes.yaml      # Vendor prefix registration
├── i2c/
│   └── vendor,device.yaml    # I2C device binding
├── spi/
│   └── vendor,spi-device.yaml
├── serial/
│   └── vendor,uart.yaml
└── ...
```

## YAML Binding Format

### Basic Structure

```yaml
# SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
%YAML 1.2
---
$id: http://devicetree.org/schemas/serial/vendor,uart.yaml#
$schema: http://devicetree.org/meta-schemas/core.yaml#

title: Vendor UART Controller

maintainers:
  - Your Name <your.email@example.com>

description: |
  The Vendor UART is a serial port controller found in Vendor SoCs.
  It supports baud rates up to 4Mbps and has a 64-byte FIFO.

properties:
  compatible:
    enum:
      - vendor,uart-v1
      - vendor,uart-v2

  reg:
    maxItems: 1

  interrupts:
    maxItems: 1

  clocks:
    maxItems: 1

  clock-names:
    const: uart

required:
  - compatible
  - reg
  - interrupts
  - clocks

additionalProperties: false

examples:
  - |
    serial@10000000 {
        compatible = "vendor,uart-v2";
        reg = <0x10000000 0x1000>;
        interrupts = <0 10 4>;
        clocks = <&clk_uart>;
        clock-names = "uart";
    };
```

## Common Property Definitions

### compatible

```yaml
# Single compatible string
compatible:
  const: vendor,device

# Multiple compatible options
compatible:
  enum:
    - vendor,device-v1
    - vendor,device-v2
    - vendor,device-v3

# Fallback compatible (specific then generic)
compatible:
  items:
    - enum:
        - vendor,device-v2
        - vendor,device-v1
    - const: vendor,device

# One of several options with fallback
compatible:
  oneOf:
    - items:
        - const: vendor,new-device
    - items:
        - const: vendor,new-device
        - const: vendor,old-device
```

### reg

```yaml
# Single memory region
reg:
  maxItems: 1

# Specific size
reg:
  items:
    - description: Control registers

# Multiple regions
reg:
  items:
    - description: Control registers
    - description: FIFO buffer

# With reg-names
reg:
  minItems: 1
  maxItems: 2

reg-names:
  minItems: 1
  maxItems: 2
  items:
    - const: regs
    - const: fifo
```

### interrupts

```yaml
# Single interrupt
interrupts:
  maxItems: 1

# Multiple interrupts
interrupts:
  items:
    - description: TX interrupt
    - description: RX interrupt

# With interrupt-names
interrupt-names:
  items:
    - const: tx
    - const: rx
```

### clocks

```yaml
# Single clock
clocks:
  maxItems: 1

clock-names:
  const: core

# Multiple clocks
clocks:
  items:
    - description: Core clock
    - description: Bus clock

clock-names:
  items:
    - const: core
    - const: bus
```

### GPIOs

```yaml
# Required GPIO
reset-gpios:
  maxItems: 1

# Optional GPIO
enable-gpios:
  maxItems: 1

# Multiple GPIOs
cs-gpios:
  minItems: 1
  maxItems: 4
```

### Regulators

```yaml
# Power supplies
vdd-supply:
  description: Main power supply

vddio-supply:
  description: I/O power supply
```

### Custom Properties

```yaml
# Vendor-prefixed properties
vendor,buffer-size:
  $ref: /schemas/types.yaml#/definitions/uint32
  description: Buffer size in bytes
  minimum: 64
  maximum: 4096
  default: 256

vendor,mode:
  $ref: /schemas/types.yaml#/definitions/string
  enum:
    - fast
    - normal
    - low-power
  default: normal

vendor,feature-enable:
  type: boolean
  description: Enable advanced feature
```

## Property Types

```yaml
# Integer
vendor,count:
  $ref: /schemas/types.yaml#/definitions/uint32

# Integer array
vendor,thresholds:
  $ref: /schemas/types.yaml#/definitions/uint32-array
  items:
    - description: Low threshold
    - description: High threshold

# String
vendor,label:
  $ref: /schemas/types.yaml#/definitions/string

# String list
vendor,modes:
  $ref: /schemas/types.yaml#/definitions/string-array

# Boolean (presence = true)
vendor,feature-enable:
  type: boolean

# phandle
vendor,external:
  $ref: /schemas/types.yaml#/definitions/phandle
```

## Child Nodes

```yaml
properties:
  # ... parent properties ...

patternProperties:
  "^channel@[0-9]+$":
    type: object
    additionalProperties: false

    properties:
      reg:
        maxItems: 1
      vendor,type:
        enum: [input, output]

    required:
      - reg
```

## Conditional Properties

```yaml
# If-then-else
if:
  properties:
    compatible:
      contains:
        const: vendor,device-v2
then:
  properties:
    vendor,new-feature:
      type: boolean
  required:
    - vendor,new-feature
```

## Dependencies

```yaml
dependencies:
  # If property A exists, property B must also exist
  vendor,dma-enable:
    required:
      - dmas
      - dma-names
```

## Complete Example

```yaml
# SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
%YAML 1.2
---
$id: http://devicetree.org/schemas/iio/adc/vendor,adc.yaml#
$schema: http://devicetree.org/meta-schemas/core.yaml#

title: Vendor Analog-to-Digital Converter

maintainers:
  - Your Name <your.email@example.com>

description: |
  The Vendor ADC is a 12-bit analog-to-digital converter with up to
  8 channels. It supports both single-ended and differential modes.

properties:
  compatible:
    enum:
      - vendor,adc-v1
      - vendor,adc-v2

  reg:
    maxItems: 1

  interrupts:
    maxItems: 1
    description: Data ready interrupt

  clocks:
    maxItems: 1

  clock-names:
    const: adc

  vref-supply:
    description: Reference voltage supply

  "#io-channel-cells":
    const: 1

  vendor,resolution:
    $ref: /schemas/types.yaml#/definitions/uint32
    enum: [8, 10, 12]
    default: 12
    description: ADC resolution in bits

  vendor,sample-rate:
    $ref: /schemas/types.yaml#/definitions/uint32
    minimum: 1000
    maximum: 1000000
    default: 100000
    description: Sample rate in Hz

patternProperties:
  "^channel@[0-7]$":
    type: object
    additionalProperties: false
    description: ADC channel configuration

    properties:
      reg:
        minimum: 0
        maximum: 7
        description: Channel number

      vendor,differential:
        type: boolean
        description: Enable differential mode

      label:
        description: Human-readable channel name

    required:
      - reg

required:
  - compatible
  - reg
  - interrupts
  - clocks
  - vref-supply
  - "#io-channel-cells"

additionalProperties: false

examples:
  - |
    #include <dt-bindings/interrupt-controller/arm-gic.h>

    adc@20000000 {
        compatible = "vendor,adc-v2";
        reg = <0x20000000 0x1000>;
        interrupts = <GIC_SPI 45 IRQ_TYPE_LEVEL_HIGH>;
        clocks = <&clk_adc>;
        clock-names = "adc";
        vref-supply = <&vdd_3v3>;
        #io-channel-cells = <1>;
        vendor,resolution = <12>;
        vendor,sample-rate = <500000>;

        channel@0 {
            reg = <0>;
            label = "voltage_input";
        };

        channel@1 {
            reg = <1>;
            vendor,differential;
            label = "current_sense";
        };
    };
```

## Validating Bindings

```bash
# Install dt-schema
pip3 install dtschema

# Validate binding document
dt-doc-validate Documentation/devicetree/bindings/iio/adc/vendor,adc.yaml

# Validate DT example against binding
dt-validate -s Documentation/devicetree/bindings/iio/adc/vendor,adc.yaml \
    arch/arm64/boot/dts/vendor/board.dts

# Kernel make target
make dt_binding_check
make dtbs_check
```

## Vendor Prefix Registration

Add your vendor to `vendor-prefixes.yaml`:

```yaml
# In Documentation/devicetree/bindings/vendor-prefixes.yaml

"^mycompany,.*":
  description: My Company Inc.
```

## Submitting Bindings

1. **Create binding document** following schema format
2. **Validate** with dt-schema tools
3. **Test** with your driver and example DTS
4. **Submit** to devicetree@vger.kernel.org (bindings) and subsystem list (driver)

```bash
# Get maintainer emails
./scripts/get_maintainer.pl Documentation/devicetree/bindings/iio/adc/vendor,adc.yaml

# Typical output:
# Rob Herring <robh@kernel.org> (maintainer:OPEN FIRMWARE AND DEVICE TREE)
# devicetree@vger.kernel.org
```

## Summary

- Modern bindings use YAML format with dt-schema
- Place bindings in `Documentation/devicetree/bindings/<subsystem>/`
- Use `$ref` for standard property types
- Include working examples
- Validate with `make dt_binding_check`
- Register new vendor prefixes
- Submit bindings with driver patches

## Next

Continue to [Part 9: I2C, SPI, and GPIO Drivers]({% link part9/index.md %}) to learn about common bus subsystems.
