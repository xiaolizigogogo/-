public class SimplePriceAdjustment {
    
    // 基于HousePriceAnalysis.java分析得出的比例系数
    private static final double 比例系数 = 0.9119;
    
    /**
     * 根据国信达单价计算调整后的中介单价
     * @param 国信达单价 国信达评估的单价 (元/㎡)
     * @return 调整后的中介单价 (元/㎡)
     */
    public static double 调整单价(double 国信达单价) {
        return 比例系数 * 国信达单价;
    }
    
    /**
     * 根据国信达总价计算调整后的中介总价
     * 注意：这个函数假设面积相同，所以总价的比例关系与单价相同
     * @param 国信达总价 国信达评估的总价 (元)
     * @return 调整后的中介总价 (元)
     */
    public static double 调整总价(double 国信达总价) {
        return 比例系数 * 国信达总价;
    }
    
    // 测试函数
    public static void main(String[] args) {
        System.out.println("=== 简化房价调整函数 ===");
        System.out.println("公式: 中介价格 = " + 比例系数 + " × 国信达价格");
        System.out.println("说明: 比例系数基本稳定，无需考虑面积参数");
        System.out.println();
        
        // 测试用例
        double[] 测试单价 = {10000, 15000, 20000, 25000, 30000};
        double[] 测试总价 = {1000000, 1500000, 2000000, 2500000, 3000000};
        
        System.out.println("单价调整测试:");
        for (double 国信达单价 : 测试单价) {
            double 调整后单价 = 调整单价(国信达单价);
            System.out.printf("国信达: %.0f元/㎡ → 中介: %.0f元/㎡%n", 
                            国信达单价, 调整后单价);
        }
        
        System.out.println("\n总价调整测试:");
        for (double 国信达总价 : 测试总价) {
            double 调整后总价 = 调整总价(国信达总价);
            System.out.printf("国信达: %.0f元 → 中介: %.0f元%n", 
                            国信达总价, 调整后总价);
        }
        
        System.out.println("\n=== 分析结论 ===");
        System.out.println("1. 比例系数平均值: 0.9136");
        System.out.println("2. 不同价格区间的比例系数基本稳定");
        System.out.println("3. 可以直接使用固定比例系数，无需考虑面积");
        System.out.println("4. 总价调整 = 比例系数 × 国信达总价");
    }
} 