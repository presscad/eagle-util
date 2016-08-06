package com.sap.nic.mytaxi.cityaccess;

/**
 * 移动招车平台出租车招车服务接入接口。该接口应由接入提供商实现。<br>
 * <b>注意：</b>实现类需要提供轻量级默认构造函数，实现应注意线程安全!
 * 
 * This interface gives access to city level taxi calling provider/vendor (e.g., 南京通用,
 * 上海大众), who is supposed to implement this interface in Java.<br>
 *
 * <br><b>接口配置：</b><br>
 * 配置文件：WEB-INFO/conf/provider_config.xml<br>
 * 配置示例：<br>
 * <pre>
 * {@code
 * <providers>
 *	<provider name="上海大众" jarPath="/jars/dazhong.jar">
 *		<interface name="IOrderService" classname="com.xxx.xxx.classname" />
 *		<interface ... </interface>
 *	</provider>
 * }
 * </pre>
 */
public interface IOrderService {
    /**
     * Get the taxi order service's capability. 
     *
     * @return the OrderServiceCapability object.
     */
	public OrderServiceCapability getCapability();

    /**
     * Create a new taxi calling order. 用户招车请求。
     *
     * @param lat latitude, required
     * @param lng longitude, required
     * @param userId user ID, required, could be user registered mobile number or some generated ID
     * @param name name of the user, can be in different forms, e.g., 张女士, 小王
     * @param gender gender of the user<br>
     * 		  0 - male, 1 - female, 2 - unknown
     * @param destLat destination latitude, optional, could be 0
     * @param destLng destination longitude, optional, could be 0
     * @param destDesc destination description. optional.<br>
     *        E.g., some name of the location or generated from map by manually pick a location
     * @param orderCarType the user requested car type description, optional
     * @param otherDesc other user request description, e.g., female driver, optional
     * @param reqMobileNum mobile number of the requested user, required
     * @param priceIncrease price increase. ignore it if price increase is not supported.<br>
     *        加价，单位：元。如果该接入商不支持加价，忽略此参数。
     * @return TaxiOrder object.
     * @throws TaxiOrderException
     */
	public TaxiOrder createOrderReq(double lat, double lng, String userId, String name, int gender,
			double destLat, double destLng, String destDesc, String orderCarType, String otherDesc,
			String reqMobileNum, float priceIncrease) throws TaxiOrderException; 

    /**
     * Query order status.
     *
     * @param orderId order ID, required
     * @return TaxiOrder object.
     * @throws TaxiOrderException
     */
	public TaxiOrder queryOrderStatus(String orderId)
			throws TaxiOrderException; 

    /**
     * Create a new taxi calling order with enlarged search range.
     *
     * @param lat latitude, required
     * @param lng longitude, required
     * @param userId user ID, required, could be user registered mobile number or some generated ID
     * @param name name of the user, can be in different forms, e.g., 张女士, 小王
     * @param gender gender of the user, 0 - male, 1 - female, 2 - unknown
     * @param destLat destination latitude, optional, could be 0
     * @param destLng destination longitude, optional, could be 0
     * @param destDesc destination description, e.g., some name of the location or generated from map
     *        by manually pick a location. optional
     * @param orderCarType the user requested car type description, optional
     * @param otherDesc other user request description, e.g., female driver, optional
     * @param reqMobileNum mobile number of the requested user, required
     * @param priceIncrease price increase. ignore it if price increase is not supported.<br>
     *        加价，单位：元。如果该接入商不支持加价，忽略此参数。
     * @return TaxiOrder object.
     * @param enlargeTime suggested time of the enlarged range (1-3). The vendor can adjust it to adapt
     *        to its own system.<br>
     *        建议的招车扩大范围倍数（一般在1-3倍）。接入提供商可以根据自己的系统做相应的映射来调整此参数
     * @return TaxiOrder object.
     * @throws UnsupportedFeatureException if enlarge search is not supported
     * @throws TaxiOrderException
     */
	public TaxiOrder enlargeSearch(double lat, double lng, String userId, String name, int gender,
			double destLat, double destLng, String destDesc, String orderCarType, String otherDesc,
			String reqMobileNum, float enlargeTime, float priceIncrease)
					throws UnsupportedFeatureException, TaxiOrderException;

    /**
     * Urge taxi driver to come fast. 催促（提醒）司机快点过来。
     *
     * @param orderId order ID, required, order should be in confirmed status
     * @param devId device ID, required
     * @param title name of the user, can be in different forms, e.g., Miss Zhang, Xiao Wang<br>
     *        可以是名字，称呼等等。
     * @param gender gender of the user, 0 - male, 1 - female, 2 - unknown
     * @param text description text from the user input from mobile devices<br>
     *        用户从移动终端输入的催车内容
     * @throws IllegalArgumentException if devId is invalid
     * @throws TaxiOrderException for other internal error, e.g., if the order is 
     *         not in confirmed status 
     */
	public void urgeTaxiDriver(String orderId, String devId, String title, int gender, String text)
			throws IllegalArgumentException, TaxiOrderException;

    /**
     * User cancels order
     *
     * @param orderId order ID, required, order should be in confirmed status
     * @param devId device ID, required
     * @throws IllegalArgumentException if orderId or devId is invalid
     * @throws TaxiOrderException for other e.g., if the order is
     *         not in confirmed status 
     */
	public void cancelOrderByUser(String orderId, String devId)
			throws IllegalArgumentException, TaxiOrderException;

    /**
     * User reports the driver does not appear after the a long time 
     *
     * @param orderId order ID, required, order should be in confirmed status
     * @param devId device ID, required
     * @throws IllegalArgumentException if orderId or devId is invalid
     * @throws TaxiOrderException for other internal error, e.g., if the order is 
     *         not in confirmed status 
     */
	public void reportDriverNoShown(String orderId, String devId)
			throws IllegalArgumentException, TaxiOrderException;

    /**
     * Query order status in batches
     *
     * @param orderIds order ID array, required
     * @return TaxiOrder object array in the same size of array orderIds.<br>
     *         If some of the order IDs is invalid or query fails, put null in the corresponding
     *         index at the returned array.<br>
     *         返回TaxiOrder[]数组。数组大小和输入的orderIds数组相同。如果对订单号，不能取到对应的订单内容，
     *         返回数组中对应的位置放入null。
     */
	public TaxiOrder[] batchQueryOrderStatus(String[] orderIds);
}
