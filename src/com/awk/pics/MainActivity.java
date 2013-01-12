package com.awk.pics;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.net.Uri;
import android.opengl.EGLContext;
import android.opengl.GLES10;
import android.os.Bundle;
import android.provider.MediaStore;
import android.provider.MediaStore.Images.Media;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnTouchListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;

public class MainActivity extends Activity {

	/**
	 * The serialization (saved instance state) Bundle key representing the
	 * current dropdown position.
	 */
	private static final String STATE_SELECTED_NAVIGATION_ITEM = "selected_navigation_item";

	private static final int CAMERA_REQUEST = 1888;
	private static final int GALLERY_REQUEST = 1889;

	// TODO: if you add a new operation, add it here, too
	private static final long INVERT_PROCESSING = 0;
	private static final long CANNY_PROCESSING = 1;
	private static final long CANNY_HOUGH_PROCESSING = 2;
	private static final long CANNY_HOUGH_RANSAC_PROCESSING = 3;
	private static final long ZEBRA_CROSSING_PROCESSING = 4;
	
	/* load our native library */
	static {
		System.loadLibrary("diblook");
	}

	private static native void processImage(Bitmap bitmap, long operation);

	private void getPictureFromGalleryIntent() {
		final Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		intent.setType("image/*");
		intent.setAction(Intent.ACTION_GET_CONTENT);
		intent.addCategory(Intent.CATEGORY_OPENABLE);
		startActivityForResult(intent, GALLERY_REQUEST);
	}

	// doesn't work properly
	private void getPictureFromCameraIntent() {
		final Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		intent.putExtra(MediaStore.EXTRA_OUTPUT,
				android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
		// intent.putExtra("return-data", true);
		startActivityForResult(intent, CAMERA_REQUEST);
	}

	Bitmap mImageBitmap = null;

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		try {
			if (resultCode == RESULT_OK) {
				if (requestCode == CAMERA_REQUEST) {

					if (mImageBitmap != null)
						mImageBitmap.recycle();

					// get the returned data
					Bundle extras = data.getExtras();
					mImageBitmap = (Bitmap) extras.get("data");

					ImageView image = (ImageView) findViewById(R.id.imageView);
					image.setImageBitmap(mImageBitmap);
				}
				if (requestCode == GALLERY_REQUEST) {
					if (mImageBitmap != null)
						mImageBitmap.recycle();

					InputStream stream = getContentResolver().openInputStream(
							data.getData());

					mImageBitmap = BitmapFactory.decodeStream(stream);

					// there's a maximum texture size that can be passed over to
					// the GPU,
					// so we need to scale the texture accordingly

					Vector2 tex_size = getMaxTextureSize();
					int width = mImageBitmap.getWidth();
					int height = mImageBitmap.getHeight();
					if (height > tex_size.y || width > tex_size.x) {
						if (width > height) {
							float scale = (float) tex_size.x / (float) width;
							mImageBitmap = Bitmap.createScaledBitmap(
									mImageBitmap,
									(int) (width * (float) scale),
									(int) (height * (float) scale), false);
						} else {
							float scale = (float) tex_size.y / (float) height;
							mImageBitmap = Bitmap.createScaledBitmap(
									mImageBitmap,
									(int) (width * (float) scale),
									(int) (height * (float) scale), false);
						}
					}

					ImageView image = (ImageView) findViewById(R.id.imageView);
					image.setImageBitmap(null);
					image.setImageBitmap(mImageBitmap);
				}
			}
		} catch (Exception e) {
			Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT).show();
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		// Set up the action bar to show a dropdown list.
		final ActionBar actionBar = getActionBar();
		actionBar.setDisplayShowTitleEnabled(true);
		actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_STANDARD);
		actionBar.setTitle("DiblookDroid");
	}

	@Override
	public void onRestoreInstanceState(Bundle savedInstanceState) {
		// Restore the previously serialized current dropdown position.
		if (savedInstanceState.containsKey(STATE_SELECTED_NAVIGATION_ITEM)) {
			getActionBar().setSelectedNavigationItem(
					savedInstanceState.getInt(STATE_SELECTED_NAVIGATION_ITEM));
		}
	}

	@Override
	public void onSaveInstanceState(Bundle outState) {
		// Serialize the current dropdown position.
		outState.putInt(STATE_SELECTED_NAVIGATION_ITEM, getActionBar()
				.getSelectedNavigationIndex());
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.picturemenu, menu);

		final ImageButton picBtn = (ImageButton) menu
				.findItem(R.id.takepic_btn).getActionView();
		picBtn.setBackgroundResource(R.drawable.camera_white);
		picBtn.setScaleX(0.7f);
		picBtn.setScaleY(1.0f);
		picBtn.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {
				// getPictureFromCameraIntent();
				getPictureFromGalleryIntent();
			}
		});

		final ImageButton effectsBtn = (ImageButton) menu.findItem(
				R.id.effects_btn).getActionView();
		effectsBtn.setBackgroundResource(R.drawable.effects);
		effectsBtn.setScaleX(1.5f);
		effectsBtn.setScaleY(1.5f);
		effectsBtn.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View v) {
				// create and fill the operations list
				final AlertDialog.Builder builder = new AlertDialog.Builder(v
						.getContext());
				builder.setTitle("Operations");
				LayoutInflater inflater = getLayoutInflater();
				View view = inflater.inflate(R.layout.operations_menu,
						(ViewGroup) findViewById(R.id.relativeLayout1));
				builder.setView(view);

				final AlertDialog alert = builder.create();

				alert.show();

				ListView lv = (ListView) alert
						.findViewById(R.id.operationsList);

				// TODO: if you add a new operation, add it here, too
				List<String> ips = new ArrayList<String>();
				ips.add("Invert");
				ips.add("Canny");
				ips.add("Hough");
				ips.add("Ransac");
				ips.add("Zebra crossing detection");

				lv.setAdapter(new ArrayAdapter<String>(lv.getContext(),
						R.layout.list_item, ips));
				lv.setOnItemClickListener(new OnItemClickListener() {
					@Override
					public void onItemClick(AdapterView<?> a, View v,
							int position, long id) {
						if (mImageBitmap == null) {
							alert.dismiss();
							Toast.makeText(v.getContext(),
									"You have to load a picture first",
									Toast.LENGTH_SHORT).show();
							return;
						}
						// TODO: if you add a new operation, add it here, too
						switch (position) {
						case 0:
							alert.dismiss();
							processImage(mImageBitmap, INVERT_PROCESSING);
							break;
						case 1:
							alert.dismiss();
							processImage(mImageBitmap, CANNY_PROCESSING);
							break;
						case 2:
							alert.dismiss();
							processImage(mImageBitmap, CANNY_HOUGH_PROCESSING);
							break;
						case 3:
							alert.dismiss();
							processImage(mImageBitmap, CANNY_HOUGH_RANSAC_PROCESSING);
							break;
						case 4:
							alert.dismiss();
							processImage(mImageBitmap, ZEBRA_CROSSING_PROCESSING);
							break;
						}
						ImageView image = (ImageView) findViewById(R.id.imageView);
						image.setImageBitmap(null);
						image.setImageBitmap(mImageBitmap);
					}
				});
			}
		});
		return true;
	}

	class Vector2 {
		public Vector2() {
			x = y = 0;
		}

		public Vector2(int sX, int sY) {
			x = sX;
			y = sY;
		}

		public int x, y;
	}

	Vector2 getMaxTextureSize() {
		// this should actually return data from OpenGl, but I can't be
		// bothered.
		// my Nexus S has a max of 2048x2048
		return new Vector2(2048, 2048);
	}
}
