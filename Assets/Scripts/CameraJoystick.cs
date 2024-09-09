using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraJoystick : MonoBehaviour
{
    [SerializeField] float mouseSensivity = 100f;
    [SerializeField] Transform Player;
    [SerializeField] Joystick LookJoystick;

    float xRotation = 0f;

    private void Update()
    {
        float mouseX = LookJoystick.Horizontal * mouseSensivity * Time.deltaTime;
        float mouseY = LookJoystick.Vertical * mouseSensivity * Time.deltaTime;

        xRotation -= mouseY;
        xRotation = Mathf.Clamp(xRotation, -90f, 90f);

        transform.localRotation = Quaternion.Euler(xRotation, 0, 0);

        Player.Rotate(Vector3.up * mouseX);
    }
}