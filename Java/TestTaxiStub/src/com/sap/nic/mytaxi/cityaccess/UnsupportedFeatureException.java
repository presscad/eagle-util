package com.sap.nic.mytaxi.cityaccess;

/**
 * This exception is thrown when requested feature is not supported.<br>
 * <b>NOTE:</b> It is preferred for the caller to check the interface's capability/feature before
 *             directly call the related function.<br>
 * <b>注意:</b>建议调用者应该先调用其它方法得到对应的interface实现的功能/能力描述，而不是直接调用对应的方法来通过捕捉异常来确定
 *            功能是否被支持。此异常被加入应用主要是为了便于开发调试。 <br>
 */
public class UnsupportedFeatureException extends RuntimeException {
	private static final long serialVersionUID = 7640123389651738446L;

	/**
	 * Constructor
	 */
	public UnsupportedFeatureException() {
        super();
    }

	/**
	 * Constructor
	 * @param description error description
	 */
    public UnsupportedFeatureException(String description) {
        super(description);
    }
}
