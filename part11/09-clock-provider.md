---
layout: default
title: "11.9 Clock Provider"
parent: "Part 11: IIO, RTC, Regulator, Clock Drivers"
nav_order: 9
---

# Clock Provider

This chapter covers implementing clock provider drivers using the Common Clock Framework.

## Basic Clock Types

### Fixed Rate Clock

```c
#include <linux/clk-provider.h>

/* Using helper */
struct clk_hw *clk_hw_register_fixed_rate(struct device *dev,
                                          const char *name,
                                          const char *parent_name,
                                          unsigned long flags,
                                          unsigned long fixed_rate);

/* From Device Tree */
osc24: oscillator {
    compatible = "fixed-clock";
    #clock-cells = <0>;
    clock-frequency = <24000000>;
};
```

### Gate Clock

```c
/* Gate clock - can only be enabled/disabled */
struct clk_hw *clk_hw_register_gate(struct device *dev,
                                    const char *name,
                                    const char *parent_name,
                                    unsigned long flags,
                                    void __iomem *reg,
                                    u8 bit_idx,
                                    u8 clk_gate_flags,
                                    spinlock_t *lock);
```

### Divider Clock

```c
/* Divider clock */
struct clk_hw *clk_hw_register_divider(struct device *dev,
                                       const char *name,
                                       const char *parent_name,
                                       unsigned long flags,
                                       void __iomem *reg,
                                       u8 shift, u8 width,
                                       u8 clk_divider_flags,
                                       spinlock_t *lock);
```

### Mux Clock

```c
/* Multiplexer clock */
struct clk_hw *clk_hw_register_mux(struct device *dev,
                                   const char *name,
                                   const char * const *parent_names,
                                   u8 num_parents,
                                   unsigned long flags,
                                   void __iomem *reg,
                                   u8 shift, u8 mask,
                                   u8 clk_mux_flags,
                                   spinlock_t *lock);
```

## Custom Clock Implementation

### Gate Clock Example

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>

struct my_clk_gate {
    struct clk_hw hw;
    void __iomem *reg;
    u8 bit_idx;
    spinlock_t *lock;
};

#define to_my_clk_gate(_hw) container_of(_hw, struct my_clk_gate, hw)

static int my_clk_gate_enable(struct clk_hw *hw)
{
    struct my_clk_gate *gate = to_my_clk_gate(hw);
    unsigned long flags;
    u32 val;

    spin_lock_irqsave(gate->lock, flags);
    val = readl(gate->reg);
    val |= BIT(gate->bit_idx);
    writel(val, gate->reg);
    spin_unlock_irqrestore(gate->lock, flags);

    return 0;
}

static void my_clk_gate_disable(struct clk_hw *hw)
{
    struct my_clk_gate *gate = to_my_clk_gate(hw);
    unsigned long flags;
    u32 val;

    spin_lock_irqsave(gate->lock, flags);
    val = readl(gate->reg);
    val &= ~BIT(gate->bit_idx);
    writel(val, gate->reg);
    spin_unlock_irqrestore(gate->lock, flags);
}

static int my_clk_gate_is_enabled(struct clk_hw *hw)
{
    struct my_clk_gate *gate = to_my_clk_gate(hw);

    return !!(readl(gate->reg) & BIT(gate->bit_idx));
}

static const struct clk_ops my_clk_gate_ops = {
    .enable = my_clk_gate_enable,
    .disable = my_clk_gate_disable,
    .is_enabled = my_clk_gate_is_enabled,
};
```

### PLL Clock Example

```c
struct my_pll {
    struct clk_hw hw;
    void __iomem *reg;
    spinlock_t *lock;
};

#define to_my_pll(_hw) container_of(_hw, struct my_pll, hw)

#define PLL_MULT_MASK   0xFF
#define PLL_MULT_SHIFT  0
#define PLL_DIV_MASK    0x0F
#define PLL_DIV_SHIFT   8
#define PLL_EN          BIT(31)

static unsigned long my_pll_recalc_rate(struct clk_hw *hw,
                                        unsigned long parent_rate)
{
    struct my_pll *pll = to_my_pll(hw);
    u32 val = readl(pll->reg);
    u32 mult = (val >> PLL_MULT_SHIFT) & PLL_MULT_MASK;
    u32 div = (val >> PLL_DIV_SHIFT) & PLL_DIV_MASK;

    if (div == 0)
        div = 1;

    return (parent_rate * mult) / div;
}

static long my_pll_round_rate(struct clk_hw *hw, unsigned long rate,
                              unsigned long *parent_rate)
{
    unsigned long mult, div;

    /* Find best mult/div for requested rate */
    div = 1;
    mult = DIV_ROUND_CLOSEST(rate, *parent_rate / div);

    if (mult > PLL_MULT_MASK)
        mult = PLL_MULT_MASK;
    if (mult < 1)
        mult = 1;

    return (*parent_rate * mult) / div;
}

static int my_pll_set_rate(struct clk_hw *hw, unsigned long rate,
                           unsigned long parent_rate)
{
    struct my_pll *pll = to_my_pll(hw);
    unsigned long flags;
    u32 val, mult, div;

    div = 1;
    mult = DIV_ROUND_CLOSEST(rate, parent_rate / div);

    spin_lock_irqsave(pll->lock, flags);

    val = readl(pll->reg);
    val &= ~((PLL_MULT_MASK << PLL_MULT_SHIFT) |
             (PLL_DIV_MASK << PLL_DIV_SHIFT));
    val |= (mult << PLL_MULT_SHIFT) | (div << PLL_DIV_SHIFT);
    writel(val, pll->reg);

    spin_unlock_irqrestore(pll->lock, flags);

    /* Wait for PLL to lock */
    usleep_range(100, 200);

    return 0;
}

static int my_pll_enable(struct clk_hw *hw)
{
    struct my_pll *pll = to_my_pll(hw);
    unsigned long flags;
    u32 val;

    spin_lock_irqsave(pll->lock, flags);
    val = readl(pll->reg);
    val |= PLL_EN;
    writel(val, pll->reg);
    spin_unlock_irqrestore(pll->lock, flags);

    /* Wait for PLL lock */
    usleep_range(100, 200);

    return 0;
}

static void my_pll_disable(struct clk_hw *hw)
{
    struct my_pll *pll = to_my_pll(hw);
    unsigned long flags;
    u32 val;

    spin_lock_irqsave(pll->lock, flags);
    val = readl(pll->reg);
    val &= ~PLL_EN;
    writel(val, pll->reg);
    spin_unlock_irqrestore(pll->lock, flags);
}

static int my_pll_is_enabled(struct clk_hw *hw)
{
    struct my_pll *pll = to_my_pll(hw);
    return !!(readl(pll->reg) & PLL_EN);
}

static const struct clk_ops my_pll_ops = {
    .recalc_rate = my_pll_recalc_rate,
    .round_rate = my_pll_round_rate,
    .set_rate = my_pll_set_rate,
    .enable = my_pll_enable,
    .disable = my_pll_disable,
    .is_enabled = my_pll_is_enabled,
};
```

## Clock Controller Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>

/* Clock IDs */
#define CLK_PLL1        0
#define CLK_CPU         1
#define CLK_AHB         2
#define CLK_APB         3
#define CLK_UART0       4
#define CLK_UART1       5
#define CLK_SPI0        6
#define CLK_I2C0        7
#define CLK_MAX         8

struct my_clk_ctrl {
    void __iomem *regs;
    spinlock_t lock;
    struct clk_hw_onecell_data *hw_data;
};

static int my_clk_ctrl_probe(struct platform_device *pdev)
{
    struct my_clk_ctrl *ctrl;
    struct clk_hw *hw;
    const char *parent;
    int ret;

    ctrl = devm_kzalloc(&pdev->dev, sizeof(*ctrl), GFP_KERNEL);
    if (!ctrl)
        return -ENOMEM;

    ctrl->regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(ctrl->regs))
        return PTR_ERR(ctrl->regs);

    spin_lock_init(&ctrl->lock);

    /* Allocate clock array */
    ctrl->hw_data = devm_kzalloc(&pdev->dev,
                                 struct_size(ctrl->hw_data, hws, CLK_MAX),
                                 GFP_KERNEL);
    if (!ctrl->hw_data)
        return -ENOMEM;

    ctrl->hw_data->num = CLK_MAX;

    /* Get reference clock from DT */
    parent = of_clk_get_parent_name(pdev->dev.of_node, 0);

    /* Register PLL */
    hw = clk_hw_register_fixed_factor(&pdev->dev, "pll1", parent,
                                      0, 100, 1);  /* x100 */
    if (IS_ERR(hw))
        return PTR_ERR(hw);
    ctrl->hw_data->hws[CLK_PLL1] = hw;

    /* Register dividers */
    hw = clk_hw_register_divider(&pdev->dev, "cpu", "pll1",
                                 CLK_SET_RATE_PARENT,
                                 ctrl->regs + 0x10, 0, 4, 0, &ctrl->lock);
    if (IS_ERR(hw))
        return PTR_ERR(hw);
    ctrl->hw_data->hws[CLK_CPU] = hw;

    hw = clk_hw_register_divider(&pdev->dev, "ahb", "pll1", 0,
                                 ctrl->regs + 0x14, 0, 4, 0, &ctrl->lock);
    if (IS_ERR(hw))
        return PTR_ERR(hw);
    ctrl->hw_data->hws[CLK_AHB] = hw;

    /* Register gate clocks */
    hw = clk_hw_register_gate(&pdev->dev, "uart0", "ahb", 0,
                              ctrl->regs + 0x20, 0, 0, &ctrl->lock);
    if (IS_ERR(hw))
        return PTR_ERR(hw);
    ctrl->hw_data->hws[CLK_UART0] = hw;

    hw = clk_hw_register_gate(&pdev->dev, "spi0", "ahb", 0,
                              ctrl->regs + 0x20, 1, 0, &ctrl->lock);
    if (IS_ERR(hw))
        return PTR_ERR(hw);
    ctrl->hw_data->hws[CLK_SPI0] = hw;

    /* Register provider */
    ret = devm_of_clk_add_hw_provider(&pdev->dev, of_clk_hw_onecell_get,
                                      ctrl->hw_data);
    if (ret)
        return ret;

    platform_set_drvdata(pdev, ctrl);

    dev_info(&pdev->dev, "Clock controller registered: %d clocks\n", CLK_MAX);
    return 0;
}

static const struct of_device_id my_clk_ctrl_of_match[] = {
    { .compatible = "vendor,clock-controller" },
    { }
};
MODULE_DEVICE_TABLE(of, my_clk_ctrl_of_match);

static struct platform_driver my_clk_ctrl_driver = {
    .probe = my_clk_ctrl_probe,
    .driver = {
        .name = "my-clock-controller",
        .of_match_table = my_clk_ctrl_of_match,
    },
};
builtin_platform_driver(my_clk_ctrl_driver);
```

## Device Tree Binding

```dts
clk_controller: clock-controller@10000000 {
    compatible = "vendor,clock-controller";
    reg = <0x10000000 0x1000>;
    #clock-cells = <1>;
    clocks = <&osc24>;
    clock-names = "ref";
};

/* Consumer */
uart0: serial@20000000 {
    compatible = "vendor,uart";
    reg = <0x20000000 0x100>;
    clocks = <&clk_controller CLK_UART0>;
    clock-names = "uart";
};
```

## Summary

- Use built-in helpers (gate, divider, mux, fixed-rate)
- Implement custom `clk_ops` for complex clocks
- Use `devm_of_clk_add_hw_provider()` for registration
- `clk_hw_onecell_data` for multiple clocks
- Device Tree defines clock hierarchy

## Further Reading

- [Clock Provider API](https://docs.kernel.org/driver-api/clk.html) - Kernel documentation
- [Clock Drivers](https://elixir.bootlin.com/linux/v6.6/source/drivers/clk) - Examples
- [Rockchip Clock](https://elixir.bootlin.com/linux/v6.6/source/drivers/clk/rockchip) - Reference

## Next

Continue to [Part 12: Network Device Drivers]({% link part12/index.md %}) to learn about network driver development.
