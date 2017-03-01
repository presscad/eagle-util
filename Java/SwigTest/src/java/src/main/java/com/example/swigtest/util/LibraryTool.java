package com.example.swigtest.util;

import java.io.*;

public class LibraryTool {
	public static class LoadException extends RuntimeException {
		public LoadException() {
		}

		public LoadException(String message) {
			super(message);
		}

		public LoadException(String message, Throwable cause) {
			super(message, cause);
		}

		public LoadException(Throwable cause) {
			super(cause);
		}

		public LoadException(String message, Throwable cause, boolean enableSuppression, boolean writableStackTrace) {
			super(message, cause, enableSuppression, writableStackTrace);
		}
	}

	enum OS {
		MAC, UNIX, WINDOWS
	}

	public static void loadLibrary(String libName, String suffix, InputStream in) throws IOException {
		File temp = File.createTempFile(libName, suffix);
		temp.deleteOnExit();

		if (!temp.exists()) {
			throw new FileNotFoundException("File " + temp.getAbsolutePath() + " or " + libName + " does not exist.");
		}
		byte[] buffer = new byte[4096];
		int readBytes;

		OutputStream out = new FileOutputStream(temp);
		try {
			while ((readBytes = in.read(buffer)) != -1) {
				out.write(buffer, 0, readBytes);
			}
		} catch (IOException e) {
			throw new IOException(e.getCause());
		} finally {
			out.close();
		}

		// Finally, load the library
		System.load(temp.getAbsolutePath());
	}

	public static void loadLibraryAsResource(String libName, String resourcePath) {
		String fileName = getPreffix() + libName + "." + getSuffix();

		String tmpPath = System.getProperty("user.dir") + "/" + fileName;
		File tmpFile = new File(tmpPath);
		if (tmpFile.exists()) {
			System.load(tmpPath);
			System.out.println("***** WARN: use tmp library file: " + tmpPath + " instead of built-in one *****");
			return;
		}

		InputStream in = LibraryTool.class.getResourceAsStream(resourcePath + "/" + fileName);
		try {
			loadLibrary(fileName, "tmp", in);
		} catch (IOException e) {
			throw new LoadException(e.getMessage());
		} finally {
			try {
				in.close();
			} catch (Exception e) {
			}
		}
	}

	public static OS getOS() {
		String os = System.getProperty("os.name").toLowerCase();
		if (os.contains("mac"))
			return OS.MAC;
		if (os.contains("nix") || os.contains("nux") || os.contains("aix"))
			return OS.UNIX;
		if (os.contains("win"))
			return OS.WINDOWS;
		return OS.UNIX;
	}

	public static String getPreffix() {
		OS os = getOS();
		switch (os) {
		case UNIX:
			return "lib";
		case WINDOWS:
			return "";
		case MAC:
			return "lib";
		}
		return "lib";
	}

	public static String getSuffix() {
		OS os = getOS();
		switch (os) {
		case UNIX:
			return "so";
		case WINDOWS:
			return "dll";
		case MAC:
			return "dylib";
		}
		return "so";
	}

	static {
		loadLibraryAsResource("swigtest", "/resources/lib");
	}

	public static void init() {
		/*
		 * do nothing, just let vm load this class, which will call above static
		 * load
		 */
	}

	public static void main(String[] args) {
		/*
		 * do nothing, just let vm load this class, which will call above static
		 * load
		 */
	}
}
