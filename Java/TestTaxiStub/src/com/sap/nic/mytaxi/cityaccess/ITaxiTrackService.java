package com.sap.nic.mytaxi.cityaccess;

/**
 * 移动招车平台出租车位置跟踪服务接口。该接口应由接入提供商实现。<br>
 * <b>注意：</b>实现类需要提供轻量级默认构造函数，实现应注意线程安全!
 * 
 * This interface gives access to city level taxi tracking service. The service provider
 * (e.g., 南京通用, 上海大众) is supposed to implement this interface in Java.<br>
 * 
 * <br><b>接口配置：</b><br>
 * 配置文件：WEB-INFO/conf/provider_config.xml<br>
 * 配置示例：<br>
 * <pre>
 * {@code
 * <providers>
 *	<provider name="上海大众" jarPath="/jars/dazhong.jar">
 *		<interface ... </interface>
 *		<interface name="ITaxiTrackService" classname="com.xxx.xxx.classname" />
 *	</provider>
 * }
 * </pre>
 */
public interface ITaxiTrackService {
    /**
 	 * Indicates whether the vendor can also return a list of taxis near by.
 	 * If enabled, #getTaxisNearBy and #trackTaxiWithNearBy are supported
 	 * 是否允许搜索指定位置附近的车
     *
     * @return false: disabled, true: enabled
     */
	public boolean trackNearbyTaxisEnabled();

    /**
     * Get the nearby taxis of the specified location.
     *
     * @param lat latitude, required
     * @param lng longitude, required
     * @param radius Suggested radius in meters. If it is left to 0, the vender will decide the radius.<br>
     *        <b>NOTE</b>: the radius is only the suggested parameter. It can be adjusted or ignored.<br>
     *        建议的搜车半径。如果传入0，接入提供商采用自己默认的半径范围。<br>
     *        <b>注意</b>：这个参数仅仅是建议的参数，接入提供商可以根据自身的系统进行调整。<br>
     * @return array of TaxiInfo object. If some of the member of TaxiInfo is not available,
     *         leave them to null or 0. If not taxi is found, return null.
     * @throws IllegalArgumentException if (lat, lng) or radius is invalid
     * @throws UnsupportedFeatureException if the vendor implementation does not support feature.
     * @throws TaxiTrackException if tracking error happens.
     */
	public TaxiInfo[] getTaxisNearBy(double lat, double lng, double radius)
			throws IllegalArgumentException, UnsupportedFeatureException, TaxiTrackException;

    /**
     * Get the nearby taxis of the specified location.
     *
     * @param devId device ID
     * @param radius Suggested radius in meters. If it is left to 0, the vender will decide the radius.
     *        <b>NOTE</b>: the radius is only the suggested parameter. It can be adjusted or ignored.
     *        建议的搜车半径。如果传入0，接入提供商采用自己默认的半径范围。
     *        <b>注意</b>：这个参数仅仅是建议的参数，接入提供商可以根据自己的系统进行调整。
     * @return array of TaxiInfo object.
     * 		   If some of the member of TaxiInfo is not available, leave them to null or 0.
     *         If no taxi is found, return null.
     * @throws IllegalArgumentException if devId or radius is invalid.
     * @throws UnsupportedFeatureException if the vendor implementation does not support feature.
     * @throws TaxiTrackException if tracking error happens.
     */
	public TaxiInfo[] trackTaxiWithNearBy(String devId, double radius) 
			throws IllegalArgumentException, UnsupportedFeatureException, TaxiTrackException;

    /**
     * Get the taxi information.
     *
     * @param devId device ID
     * @return TaxiInfo object.
     * 		   If some of the member of TaxiInfo is not available, leave them to null or 0.
     * @throws IllegalArgumentException if devId or radius is invalid.
     * @throws TaxiTrackException if tracking error happens.
     */
	public TaxiInfo trackTaxi(String devId)
			throws IllegalArgumentException, TaxiTrackException;
}
