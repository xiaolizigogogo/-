public class PriceAdjustmentFunction {
    
    // 根据数据分析得出的比例系数 (过原点线性回归)
    private static final double 比例系数 = 0.9119; // 基于367个数据点的分析结果
    
    /**
     * 根据国信达单价计算调整后的中介单价
     * @param 国信达单价 国信达评估的单价 (元/㎡)
     * @return 调整后的中介单价 (元/㎡)
     */
    public static double 计算调整后单价(double 国信达单价) {
        return 比例系数 * 国信达单价;
    }
    
    /**
     * 根据国信达总价和面积计算调整后的中介总价
     * @param 国信达总价 国信达评估的总价 (元)
     * @param 面积 房屋面积 (㎡)
     * @return 调整后的中介总价 (元)
     */
    public static double 计算调整后总价(double 国信达总价, double 面积) {
        double 国信达单价 = 国信达总价 / 面积;
        double 调整后单价 = 计算调整后单价(国信达单价);
        return 调整后单价 * 面积;
    }
    
    /**
     * 根据国信达单价和面积计算调整后的中介总价
     * @param 国信达单价 国信达评估的单价 (元/㎡)
     * @param 面积 房屋面积 (㎡)
     * @return 调整后的中介总价 (元)
     */
    public static double 根据单价计算调整后总价(double 国信达单价, double 面积) {
        double 调整后单价 = 计算调整后单价(国信达单价);
        return 调整后单价 * 面积;
    }
    
    // 测试函数
    public static void main(String[] args) {
        System.out.println("=== 房价调整函数测试 ===");
        System.out.println("使用比例系数: " + 比例系数);
        System.out.println();
        
        // 测试用例
        double[] 测试单价 = {10000, 15000, 20000, 25000, 30000};
        double 测试面积 = 100.0; // 100㎡
        
        System.out.println("面积: " + 测试面积 + "㎡");
        System.out.println();
        
        for (double 国信达单价 : 测试单价) {
            double 调整后单价 = 计算调整后单价(国信达单价);
            double 国信达总价 = 国信达单价 * 测试面积;
            double 调整后总价 = 根据单价计算调整后总价(国信达单价, 测试面积);
            
            System.out.printf("国信达单价: %.0f元/㎡ → 调整后单价: %.0f元/㎡%n", 
                            国信达单价, 调整后单价);
            System.out.printf("国信达总价: %.0f元 → 调整后总价: %.0f元%n", 
                            国信达总价, 调整后总价);
            System.out.println();
        }
        
        // 直接使用总价的测试
        System.out.println("=== 直接使用总价的测试 ===");
        double 测试总价 = 2000000; // 200万
        double 调整后总价 = 计算调整后总价(测试总价, 测试面积);
        System.out.printf("国信达总价: %.0f元 → 调整后总价: %.0f元%n", 
                        测试总价, 调整后总价);
    }
} 