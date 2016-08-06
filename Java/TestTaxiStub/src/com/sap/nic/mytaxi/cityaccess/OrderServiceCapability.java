package com.sap.nic.mytaxi.cityaccess;

/**
 * 接入提供商招车相关服务的能力（支持的招车的具体功能）。<br>
 * 
 * This interface defines the ordering capability (supported features) of the vendor/provider.<br>
 */
public class OrderServiceCapability {
	/**
	 * Price increase mode: do not support price increase<br>
	 * 加价模式：不允许加价
	 */
	public static final int PRICE_MODE_INCREASE_FORBIDDEN = 0;
	/**
	 * Price increase mode: support fixed price increase with (1 to n levels)<br>
	 * 加价模式：允许固定加价，可以有一个或者多个固定价格
	 */
	public static final int PRICE_MODE_FIXED_INCREASE = 1;
	/**
	 * Price increase mode: price increase can be any number<br>
	 * 加价模式：无固定加价，加价不限
	 */
	public static final int PRICE_MODE_ANY_INCREASE = 2;

	/**
	 * Price increase mode. 加价模式<br>
	 * @see #PRICE_MODE_INCREASE_FORBIDDEN
	 * @see #PRICE_MODE_FIXED_INCREASE
	 * @see #PRICE_MODE_ANY_INCREASE
	 */
	public int priceIncreaseMode;

	/**
	 * For fixed price increase mode, number of the supported price increase levels<br>
	 * 对于固定加价模式，有几种固定加价<br>
	 * 如果接入商不支持固定加价，该字段设为0<br>
	 */
	public int fixedPriceIncreaseLevelNumber;

	/**
	 * For fixed price increase, the corresponding price increase value array.<br>
	 * 固定加价所对应的加价数组，单位：元<br>
	 * 例如，某提供商支持两种加价，5元和10元，代码示例：<br>
	 * <pre>
	 * {@code
	 *     fixedPriceIncreaseLevelNumber = 2;
	 *     fixedPriceIncreases = new float[fixedPriceIncreaseLevelNumber];
	 *     fixedPriceIncreases[0] = 5;
	 *     fixedPriceIncreases[1] = 10;
	 * }
	 * </pre>
	 * 如果接入商不支持固定加价，字段可以不填写，设为null<br>
	 */
	public float[] fixedPriceIncreases;

	/**
	 * indicates whether enlarge search is supported<br>
	 * 是否允许扩大招车范围
	 */
	public boolean enlargeSearchEnabled;

    /**
 	 * indicates whether the urge taxi driver is supported.
 	 * 是否允许催车（提醒司机）
 	 */
	public boolean urgeTaxiEnabled;
}
