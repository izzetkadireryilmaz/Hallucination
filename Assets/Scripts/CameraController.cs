using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class CameraController : MonoBehaviour
{
    public Transform Player;
    public float MouseSensitivity = 2f;
    public float cameraVerticalRotation = 0f;
    public float cameraHorizontalRotation = 0f;

    private void Start()
    {
        Cursor.visible = false;
    }
    void Update()
    {
        // kamera de�erinin ayarlad���m�z mouse hassasiyeti ile hareket etmesini sa�l�yoruz.
        float inputX = Input.GetAxis("Mouse X") * MouseSensitivity;
        float inputY = Input.GetAxis("Mouse Y") * MouseSensitivity;

        // kameran�n dikey hareketini sa�l�yor ve bunu 90 derece ile s�n�rl�yoruz.
        cameraVerticalRotation -= inputY;
        cameraVerticalRotation = Mathf.Clamp(cameraVerticalRotation, -90f, 90f);
        transform.localEulerAngles = Vector3.right * cameraVerticalRotation;

        // kameran�n yatay hareketi i�in karakterin d�nmesini sa�l�yoruz.
        Player.Rotate(Vector3.up * inputX);
    }

}