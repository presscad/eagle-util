package com.sap.nic.mytaxi.cityaccess;

/**
 * This exception is thrown when error related to taxi order happens
 */
public class TaxiOrderException extends RuntimeException {
	/**
	 * Error No: device ID is not found<br>
	 * description should contain the device ID.<br>
	 * 错误代码：设备标志没有找到。description参数中应包含出错的设备标志。
	 */
	public static final int ERRNO_DEVICE_NOT_FOUND = 100;

	/**
	 * Error No: order not found<br>
	 * description should contain the order ID.<br>
	 * 错误代码：订单号没有找到。description参数中应包含该订单号。
	 */
	public static final int ERRNO_ORDER_NOT_FOUND = 101;

	/**
	 * Error No: invalid order operation<br>
	 * description should contain the order state and why the operation is invalid.<br>
	 * 错误代码：操作不合理，例如用户取消招车，但是当前订单不是司机确认状态。<br>
	 * description参数中应包含订单状态，用户操作，简单描述为什么操作不合理。
	 */
	public static final int ERRNO_INVALID_ORDER_OP = 102;

	/**
	 * Error No: database operation error.<br>
	 * description should contain details.<br>
	 * 错误代码：数据库错误。<br>
	 * description参数中应包含具体的错误信息。
	 */
	public static final int ERRNO_DB_OPERATION_ERROR = 103;

	/**
	 * Error No: network error.<br>
	 * description should contain details.<br>
	 * 错误代码：网络错误。<br>
	 * description参数中应包含具体的错误信息。
	 */
	public static final int ERRNO_NETWORK_ERROR = 104;


	// add new error numbers above

	private static final long serialVersionUID = -4107483338734046173L;
	protected long errorCode;

	/**
	 * Constructor
	 * 
	 * @param errorCode error code for taxi ordering
	 */
	public TaxiOrderException(long errorCode) {
        super();
        this.errorCode = errorCode;
    }

	/**
	 * Constructor
	 * 
	 * @param errorCode error code for taxi ordering
	 * @param description error description
	 */
	public TaxiOrderException(long errorCode, String description) {
        super(description);
        this.errorCode = errorCode;
    }
}
