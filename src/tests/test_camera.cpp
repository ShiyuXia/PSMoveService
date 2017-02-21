#include "PSEyeVideoCapture.h"
#include "ClientConstants.h"
#include "opencv2/opencv.hpp"
#include <algorithm>
#include <vector>

const std::vector<int> known_keys = {113, 97, 119, 115, 101, 100, 114, 102, 116, 103, 121, 104};
// q, a, w, s, e, d, r, f, t, g, y, h

const std::vector<int> known_keys_check = { 32 };
// SPACEBAR

struct camera_state
{
    PSEyeVideoCapture *camera;
    std::string identifier;
};

int main(int, char**)
{
    std::vector<camera_state> camera_states;

    // Open all available cameras (up to 4 max)
	for (int camera_index = 0; camera_index < PSMOVESERVICE_MAX_TRACKER_COUNT; ++camera_index)
	{
        PSEyeVideoCapture *camera = new PSEyeVideoCapture(camera_index); // open the default camera

        if (camera->isOpened())
        {
            std::string identifier = camera->getUniqueIndentifier();

            camera_states.push_back({ camera, identifier });
        }
        else
        {
            delete camera;
        }
    }
    
    // Create a window for each opened camera
    std::for_each(
        camera_states.begin(), 
        camera_states.end(),
        [&camera_states](camera_state &state) {
            cv::namedWindow(state.identifier.c_str(), 1);
        }
    );

    bool bKeepRunning = camera_states.size() > 0;
    while (bKeepRunning)
    {
        // Render each camera frame in it's own window
        std::for_each(
            camera_states.begin(),
            camera_states.end(),
            [&camera_states](camera_state &state) {
                cv::Mat frame;
                (*state.camera) >> frame; // get a new frame from camera

                if (!frame.empty())
                {
                    imshow(state.identifier.c_str(), frame);
                }
            }
        );

        int wk = cv::waitKey(10);

        if (wk == 27)  // Escape
        {
            bKeepRunning = false;
        }
        else if (std::find(known_keys.begin(), known_keys.end(), wk) != known_keys.end())
        {
            int cap_prop = CV_CAP_PROP_EXPOSURE;
            double val_diff = 0;
            std::string prop_str("CV_CAP_PROP_EXPOSURE");
            
            // q/a for +/- exposure
            // CL Eye [0 255]
            if ((wk == 113) || (wk == 97))
            {
                cap_prop = CV_CAP_PROP_EXPOSURE;
                prop_str = "CV_CAP_PROP_EXPOSURE";
                val_diff = (wk == 113) ? 10 : -10;
            }

            // w/s for +/- contrast
            // Note that, for CL EYE at least, changing contrast ALSO changes exposure
            // CL Eye [0 255]
            if ((wk == 119) || (wk == 115))
            {
                cap_prop = CV_CAP_PROP_CONTRAST;
                prop_str = "CV_CAP_PROP_CONTRAST";
                val_diff = (wk == 119) ? 1 : -1;
            }

            // e/d for +/- gain
            // For CL Eye, gain seems to be changing colour balance. 
            if ((wk == 101) || (wk == 100))
            {
                cap_prop = CV_CAP_PROP_GAIN;
                prop_str = "CV_CAP_PROP_GAIN";
                val_diff = (wk == 101) ? 4 : -4;
            }

            // r/f for +/- hue
            if ((wk == 114) || (wk == 102))
            {
                cap_prop = CV_CAP_PROP_HUE;
                prop_str = "CV_CAP_PROP_HUE";
                val_diff = (wk == 114) ? 1 : -1;
            }
            
            // t/g for +/- sharpness
            // For CL_Eye, 1 - sharpness
            if ((wk == 116) || (wk == 103))
            {
                cap_prop = CV_CAP_PROP_SHARPNESS;
                prop_str = "CV_CAP_PROP_SHARPNESS";
                val_diff = (wk == 116) ? 1 : -1;
            }

			// y/h for +/- frame rate
			// For CL_Eye, don't know
			if ((wk == 121) || (wk == 104))
			{
				cap_prop = CV_CAP_PROP_FPS;
				prop_str = "CV_CAP_PROP_FPS";
				val_diff = (wk == 121) ? 10 : -10;
			}

            std::for_each(
                camera_states.begin(), 
                camera_states.end(),
                [&camera_states, &cap_prop, &prop_str, &val_diff](camera_state &state) {

                    double val = state.camera->get(cap_prop);
                    std::cout << state.identifier << ": Value of " << prop_str << " was " << val << std::endl;

					switch (cap_prop)
					{
					case CV_CAP_PROP_FPS:
						if (val == 2) { if (val_diff > 0) val_diff = 1; else val_diff = 0; }
						else if (val == 3) { if (val_diff > 0) val_diff = 2; else val_diff = -1; }
						else if (val == 5) { if (val_diff > 0) val_diff = 3; else val_diff = -2; }
						else if (val == 8) { if (val_diff > 0) val_diff = 2; else val_diff = -3; }
						else if (val == 10) { if (val_diff > 0) val_diff = 5; else val_diff = -2; }
						else if (val == 15) { if (val_diff > 0) val_diff = 5; else val_diff = -5; }
						else if (val == 20) { if (val_diff > 0) val_diff = 5; else val_diff = -5; }
						else if (val == 25) { if (val_diff > 0) val_diff = 5; else val_diff = -5; }
						else if (val == 30) { if (val_diff < 0) { val_diff = -5; } }
						else if (val == 60) { if (val_diff > 0) { val_diff = 15; } }
						else if (val == 75) { if (val_diff > 0) val_diff = 8; else val_diff = -15; }
						else if (val == 83) { if (val_diff < 0) val_diff = -8; }
					}

					val += val_diff;
                    state.camera->set(cap_prop, val);

                    val = state.camera->get(cap_prop);
                    std::cout << state.identifier << ": Value of " << prop_str << " changed by " << val_diff << " and is now " << val << std::endl;
                }
            );
        }
		else if (std::find(known_keys_check.begin(), known_keys_check.end(), wk) != known_keys_check.end())
		{
			int cap_prop = CV_CAP_PROP_FPS;
			std::string prop_str("CV_CAP_PROP_FPS");

			// SPACEBAR to check frame rate
			if ((wk == 32))
			{
				cap_prop = CV_CAP_PROP_FPS;
				prop_str = "CV_CAP_PROP_FPS";
			}

			std::for_each(
				camera_states.begin(),
				camera_states.end(),
				[&camera_states, &cap_prop, &prop_str](camera_state &state) {

				double val = state.camera->get(cap_prop);
				std::cout << state.identifier << ": Value of " << prop_str << " is " << val << std::endl;
			}
			);
		}
        else if (wk > 0)
        {
            std::cout << "Unknown key has code " << wk << std::endl;
        }
    }

    // Disconnect all active cameras
    std::for_each(
        camera_states.begin(), 
        camera_states.end(),
        [&camera_states](camera_state &state) {
            delete state.camera;
        }
    );
    camera_states.clear();

    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}
