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
        // kamera deðerinin ayarladýðýmýz mouse hassasiyeti ile hareket etmesini saðlýyoruz.
        float inputX = Input.GetAxis("Mouse X") * MouseSensitivity;
        float inputY = Input.GetAxis("Mouse Y") * MouseSensitivity;

        // kameranýn dikey hareketini saðlýyor ve bunu 90 derece ile sýnýrlýyoruz.
        cameraVerticalRotation -= inputY;
        cameraVerticalRotation = Mathf.Clamp(cameraVerticalRotation, -90f, 90f);
        transform.localEulerAngles = Vector3.right * cameraVerticalRotation;

        // kameranýn yatay hareketi için karakterin dönmesini saðlýyoruz.
        Player.Rotate(Vector3.up * inputX);
    }

}